#!/usr/bin/env python3
"""
Multi-Variable Snapshot Correctness Experiment
===============================================
Tests how snapshot correctness is affected by:
  - Number of vehicles (NPCs): 0, 1, 2
  - Snapshot skip length: 50, 100, 150 ticks

For each combination, runs: recording → copy → baseline → snapshot → analyze

Usage:
  conda activate carla14
  python tmpcode/batch_correctness_experiment.py
"""

import carla
import os
import sys
import time
import shutil
import glob
import csv
import random
import numpy as np
from datetime import datetime

# Paths
CARLA_EXAMPLES_DIR = "/S980PRO/xinyu/Documents/carla/PythonAPI/examples"
DATA_DIR = os.path.join(CARLA_EXAMPLES_DIR, "data")
CONFIG_DIR = os.path.join(CARLA_EXAMPLES_DIR, "config")
BATCH_DIR = os.path.join(DATA_DIR, "batch_experiment")
MYAB_FILE = os.path.join(CONFIG_DIR, "myAB.txt")
MYTOTAL_FILE = os.path.join(CONFIG_DIR, "myTotalVehicles.txt")

HARDCODED_CSV_PATHS = {
    3: [os.path.join(DATA_DIR, f"20250315_16294{5+i}_UWheeledVehicle4W_{i}.csv") for i in range(3)],
    2: [os.path.join(DATA_DIR, "20250315_163750_UWheeledVehicle4W_0.csv"),
        os.path.join(DATA_DIR, "20250315_163750_UWheeledVehicle4W_1.csv")],
    1: [os.path.join(DATA_DIR, "20250104_234409_UWheeledVehicle4W.csv")],
}

TOTAL_TICKS = 300

# ============================================================
def write_myab(a, b):
    os.makedirs(os.path.dirname(MYAB_FILE), exist_ok=True)
    with open(MYAB_FILE, 'w') as f:
        f.write(f"{a}\n{b}\n")

def set_vehicle_count(n):
    with open(MYTOTAL_FILE, 'w') as f:
        f.write(str(n))

def backup_hardcoded_csvs(total_v):
    if total_v in HARDCODED_CSV_PATHS:
        for tp in HARDCODED_CSV_PATHS[total_v]:
            if os.path.exists(tp):
                shutil.copy2(tp, tp + ".batch_backup")

def copy_csvs_for_replay(total_v, sources):
    if total_v not in HARDCODED_CSV_PATHS:
        return False
    targets = HARDCODED_CSV_PATHS[total_v]
    src_by_id = {}
    for s in sources:
        bn = os.path.basename(s).replace('.csv', '')
        parts = bn.rsplit('_', 1)
        vid = int(parts[1]) if (len(parts) == 2 and parts[1].isdigit()) else 0
        src_by_id[vid] = s
    for tp in targets:
        tp_parts = os.path.basename(tp).replace('.csv', '').rsplit('_', 1)
        tvid = int(tp_parts[1]) if (len(tp_parts) == 2 and tp_parts[1].isdigit()) else 0
        if tvid in src_by_id:
            shutil.copy2(src_by_id[tvid], tp)
    return True

# ============================================================
class TrajectoryRecorder:
    def __init__(self, out_dir, label):
        self.out_dir = out_dir
        self.label = label
        self.rows = []
    def record(self, vehicle, tick):
        t = vehicle.get_transform()
        self.rows.append({
            'frame_count': tick,
            'location_x': round(t.location.x, 4),
            'location_y': round(t.location.y, 4),
            'location_z': round(t.location.z, 4),
            'rotation_yaw': round(t.rotation.yaw, 4),
        })
    def save(self):
        os.makedirs(self.out_dir, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        fp = os.path.join(self.out_dir, f"{ts}_{self.label}.csv")
        if self.rows:
            with open(fp, 'w', newline='') as f:
                w = csv.DictWriter(f, fieldnames=list(self.rows[0].keys()))
                w.writeheader()
                w.writerows(self.rows)
        return fp

def spawn_npc(world, num):
    bp_lib = world.get_blueprint_library()
    sps = world.get_map().get_spawn_points()
    vehicles = []
    for i in range(num):
        bp = random.choice(bp_lib.filter('vehicle'))
        sp = sps[i % len(sps)]
        if i >= len(sps):
            sp.location += carla.Location(x=random.uniform(-3,3), y=random.uniform(-3,3), z=0.2)
        v = world.spawn_actor(bp, sp)
        ctrl = carla.VehicleControl()
        ctrl.throttle = 0.5 + 0.1 * (i % 3)
        ctrl.steer = 0.05 * (i % 3 - 1)
        v.apply_control(ctrl)
        vehicles.append(v)
    return vehicles

def run_one_sim(client, label, snap_a, snap_b, out_dir, num_npcs):
    snap_enabled = snap_a < 1000
    write_myab(snap_a, snap_b)
    time.sleep(0.2)

    world = client.get_world()
    orig_settings = world.get_settings()
    s = world.get_settings()
    s.synchronous_mode = True
    s.fixed_delta_seconds = 0.05
    world.apply_settings(s)

    vehicles = []
    recorder = TrajectoryRecorder(out_dir, label)
    try:
        bp = world.get_blueprint_library().filter('charger_2020')[0]
        ego = world.spawn_actor(bp, world.get_map().get_spawn_points()[-1])
        vehicles.append(ego)
        ego.apply_control(carla.VehicleControl(throttle=0.6))

        npcs = spawn_npc(world, num_npcs)
        vehicles.extend(npcs)

        cnt = 0
        while cnt < TOTAL_TICKS:
            if snap_enabled and cnt == snap_a:
                cnt = snap_b - 1
                if snap_b > TOTAL_TICKS:
                    break
            world.tick()
            cnt += 1
            recorder.record(ego, cnt)

        csv_path = recorder.save()
    except Exception as e:
        print(f"  [ERR] {e}")
        import traceback; traceback.print_exc()
        csv_path = None
    finally:
        client.apply_batch([carla.command.DestroyActor(x) for x in vehicles])
        for _ in range(5):
            world.tick()
        world.apply_settings(orig_settings)
        write_myab(1000, 12000)
    return csv_path

# ============================================================
def analyze(baseline_fp, snapshot_fp, snap_end):
    def load(fp):
        rows = {}
        with open(fp) as f:
            for r in csv.DictReader(f):
                fr = int(float(r['frame_count']))
                rows[fr] = {'x': float(r['location_x']), 'y': float(r['location_y']),
                            'z': float(r['location_z']), 'yaw': float(r['rotation_yaw'])}
        return rows
    b = load(baseline_fp)
    s = load(snapshot_fp)
    common = sorted(set(b) & set(s))
    post = [f for f in common if f >= snap_end]
    if not post:
        return {'mean_mm': -1, 'max_mm': -1, 'std_mm': -1}
    eucs = [np.sqrt((b[f]['x']-s[f]['x'])**2 + (b[f]['y']-s[f]['y'])**2 + (b[f]['z']-s[f]['z'])**2) for f in post]
    eucs = np.array(eucs)
    # Check if error grows
    first_half = eucs[:len(eucs)//2].mean() if len(eucs) > 1 else eucs[0]
    second_half = eucs[len(eucs)//2:].mean() if len(eucs) > 1 else eucs[0]
    return {
        'mean_mm': eucs.mean() * 1000,
        'max_mm': eucs.max() * 1000,
        'std_mm': eucs.std() * 1000,
        'first_mm': eucs[0] * 1000,
        'last_mm': eucs[-1] * 1000,
        'trend': 'converging' if second_half < first_half else ('growing' if second_half > first_half * 1.2 else 'stable'),
    }

# ============================================================
def main():
    os.makedirs(BATCH_DIR, exist_ok=True)
    random.seed(10)
    np.random.seed(10)

    client = carla.Client('127.0.0.1', 2000)
    client.set_timeout(60)

    # Experiment grid
    npcs_list = [0, 1, 2]       # NPC count
    skip_lens = [50, 100, 150]  # tick skip length
    snap_mid = 100               # snapshot starts at tick 100

    all_results = []

    for num_npcs in npcs_list:
        total_v = num_npcs + 1
        set_vehicle_count(num_npcs)
        backup_hardcoded_csvs(total_v)
        print(f"\n{'#'*60}\n# VEHICLES: {total_v} ({num_npcs} NPCs + 1 EGO)\n{'#'*60}")

        # Recording run (shared for all skip lengths with same vehicle count)
        print("\n-- Recording run --")
        t0 = time.time()
        rec_csv = run_one_sim(client, f"REC_V{total_v}", 1000, 12000, BATCH_DIR, num_npcs)
        if not rec_csv:
            print("[FATAL] Recording failed!"); continue

        time.sleep(1)
        all_uw = glob.glob(os.path.join(DATA_DIR, "*_UWheeledVehicle4W_*.csv")) + \
                 glob.glob(os.path.join(DATA_DIR, "*_UWheeledVehicle4W.csv"))
        new_uw = [f for f in all_uw if os.path.getmtime(f) > t0]
        print(f"  UWheeledVehicle4W CSVs: {len(new_uw)}")
        copy_csvs_for_replay(total_v, new_uw)
        time.sleep(0.3)

        # Baseline (shared)
        print("\n-- Baseline run --")
        baseline_csv = run_one_sim(client, f"BASE_V{total_v}", 1000, 12000, BATCH_DIR, num_npcs)
        if not baseline_csv:
            print("[FATAL] Baseline failed!"); continue

        # Snapshot runs with different skip lengths
        for skip_len in skip_lens:
            snap_start = snap_mid
            snap_end = snap_mid + skip_len
            if snap_end > TOTAL_TICKS:
                continue

            label = f"SNAP_V{total_v}_L{skip_len}"
            print(f"\n-- Snapshot run: {label} [{snap_start}, {snap_end}) --")
            snap_csv = run_one_sim(client, label, snap_start, snap_end, BATCH_DIR, num_npcs)
            if not snap_csv:
                print("  [WARN] Snapshot failed, skipping"); continue

            # Analyze
            result = analyze(baseline_csv, snap_csv, snap_end)
            result['vehicles'] = total_v
            result['skip_len'] = skip_len
            result['baseline'] = os.path.basename(baseline_csv)
            result['snapshot'] = os.path.basename(snap_csv)
            all_results.append(result)

            print(f"  Result: mean={result['mean_mm']:.2f}mm, max={result['max_mm']:.2f}mm, "
                  f"trend={result['trend']}")

    # ================================================================
    # Summary
    # ================================================================
    print(f"\n{'='*70}")
    print(f"BATCH EXPERIMENT SUMMARY")
    print(f"{'='*70}")
    print(f"{'Vehicles':>8s} {'Skip':>6s} {'Mean(mm)':>10s} {'Max(mm)':>10s} {'Std(mm)':>10s} {'Trend':>12s}")
    print("-" * 60)
    for r in sorted(all_results, key=lambda x: (x['vehicles'], x['skip_len'])):
        print(f"{r['vehicles']:8d} {r['skip_len']:6d} {r['mean_mm']:10.2f} {r['max_mm']:10.2f} {r['std_mm']:10.2f} {r['trend']:>12s}")

    # Save detailed results
    summary_path = os.path.join(BATCH_DIR, "batch_results.csv")
    with open(summary_path, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=['vehicles', 'skip_len', 'mean_mm', 'max_mm', 'std_mm', 'first_mm', 'last_mm', 'trend'])
        w.writeheader()
        for r in all_results:
            w.writerow({k: r[k] for k in w.fieldnames})
    print(f"\nDetailed results: {summary_path}")

    # Reset
    write_myab(1000, 12000)
    set_vehicle_count(2)

    return 0

if __name__ == "__main__":
    sys.exit(main())
