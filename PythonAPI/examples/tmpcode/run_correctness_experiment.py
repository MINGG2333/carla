#!/usr/bin/env python3
"""
Snapshot Correctness Experiment — Self-Contained Version
=========================================================
Runs the complete correctness experiment for ASE 2026 Rebuttal.

Flow:
  Run 1 (RECORDING): Normal sim → C++ saves UWheeledVehicle4W CSVs
  Copy CSVs to hardcoded paths expected by C++ VehicleDynamicsReader
  Run 2 (BASELINE):  Normal sim → Python records trajectory (BASELINE)
  Run 3 (SNAPSHOT):  Snapshot sim [SNAP_START, SNAP_END) → Python records trajectory (SNAPSHOT)
  Analyze: Compare BASELINE vs SNAPSHOT post-snapshot segment

Usage:
  conda activate carla14
  python tmpcode/run_correctness_experiment.py
"""

import carla
import argparse
import os
import sys
import time
import shutil
import glob
import csv
import random
import numpy as np
from datetime import datetime

# ============================================================
# Paths
# ============================================================
CARLA_EXAMPLES_DIR = "/S980PRO/xinyu/Documents/carla/PythonAPI/examples"
DATA_DIR = os.path.join(CARLA_EXAMPLES_DIR, "data")
CONFIG_DIR = os.path.join(CARLA_EXAMPLES_DIR, "config")
EXPERIMENT_DIR = os.path.join(DATA_DIR, "correctness_experiment")

MYAB_FILE = os.path.join(CONFIG_DIR, "myAB.txt")
MYTOTAL_FILE = os.path.join(CONFIG_DIR, "myTotalVehicles.txt")

# Hardcoded CSV paths in C++ WheeledVehicleMovementComponent constructor
HARDCODED_CSV_PATHS = {
    3: [
        os.path.join(DATA_DIR, "20250315_162945_UWheeledVehicle4W_0.csv"),
        os.path.join(DATA_DIR, "20250315_162946_UWheeledVehicle4W_1.csv"),
        os.path.join(DATA_DIR, "20250315_162946_UWheeledVehicle4W_2.csv"),
    ],
    4: [
        os.path.join(DATA_DIR, "20250315_163406_UWheeledVehicle4W_0.csv"),
        os.path.join(DATA_DIR, "20250315_163407_UWheeledVehicle4W_1.csv"),
        os.path.join(DATA_DIR, "20250315_163407_UWheeledVehicle4W_2.csv"),
        os.path.join(DATA_DIR, "20250315_163407_UWheeledVehicle4W_3.csv"),
    ],
    2: [
        os.path.join(DATA_DIR, "20250315_163750_UWheeledVehicle4W_0.csv"),
        os.path.join(DATA_DIR, "20250315_163750_UWheeledVehicle4W_1.csv"),
    ],
    1: [
        os.path.join(DATA_DIR, "20250104_234409_UWheeledVehicle4W.csv"),
    ],
}

# ============================================================
# Config helpers
# ============================================================

def read_int_file(filepath, default=0):
    try:
        with open(filepath, 'r') as f:
            return int(f.read().strip())
    except:
        return default


def write_myab(a, b):
    os.makedirs(os.path.dirname(MYAB_FILE), exist_ok=True)
    with open(MYAB_FILE, 'w') as f:
        f.write(f"{a}\n{b}\n")
    print(f"  [CONFIG] myAB.txt ← ({a}, {b})")


def get_total_vehicles():
    return read_int_file(MYTOTAL_FILE, 2) + 1


# ============================================================
# CSV helpers
# ============================================================

def backup_hardcoded_csvs(total_vehicles):
    if total_vehicles not in HARDCODED_CSV_PATHS:
        return
    for tp in HARDCODED_CSV_PATHS[total_vehicles]:
        if os.path.exists(tp):
            backup = tp + ".backup_experiment"
            shutil.copy2(tp, backup)
            print(f"  [BACKUP] {os.path.basename(tp)}")


def copy_csvs_for_replay(total_vehicles, source_csvs):
    if total_vehicles not in HARDCODED_CSV_PATHS:
        print(f"  [WARN] No hardcoded paths for {total_vehicles} vehicles")
        return False

    target_paths = HARDCODED_CSV_PATHS[total_vehicles]

    source_by_id = {}
    for src in source_csvs:
        basename = os.path.basename(src).replace('.csv', '')
        parts = basename.rsplit('_', 1)
        if len(parts) == 2 and parts[1].isdigit():
            source_by_id[int(parts[1])] = src
        else:
            source_by_id[0] = src

    print(f"  Source CSVs by vehicle ID: {dict(sorted(source_by_id.items()))}")

    for tp in target_paths:
        tp_basename = os.path.basename(tp).replace('.csv', '')
        tp_parts = tp_basename.rsplit('_', 1)
        target_vid = int(tp_parts[1]) if (len(tp_parts) == 2 and tp_parts[1].isdigit()) else 0

        if target_vid in source_by_id:
            shutil.copy2(source_by_id[target_vid], tp)
            print(f"  [COPY] vehicle[{target_vid}] → {os.path.basename(tp)}")
        else:
            print(f"  [WARN] No source CSV for vehicle ID {target_vid}")

    return True


# ============================================================
# Trajectory Recorder (lightweight, no pygame)
# ============================================================

class TrajectoryRecorder:
    def __init__(self, output_dir, label):
        self.output_dir = output_dir
        self.label = label
        self.rows = []

    def record(self, vehicle, tick_count, tick_time_ms):
        t = vehicle.get_transform()
        row = {
            'location_x': round(t.location.x, 4),
            'location_y': round(t.location.y, 4),
            'location_z': round(t.location.z, 4),
            'rotation_yaw': round(t.rotation.yaw, 4),
            'heading': self._heading(t.rotation.yaw),
            'frame_count': tick_count,
            'tick_time': tick_time_ms,
            'current_time': round(time.time(), 4),
        }
        self.rows.append(row)

    @staticmethod
    def _heading(yaw):
        h = 'N' if abs(yaw) < 89.5 else ''
        h += 'S' if abs(yaw) > 90.5 else ''
        h += 'E' if 179.5 > yaw > 0.5 else ''
        h += 'W' if -0.5 > yaw > -179.5 else ''
        return h

    def save(self):
        os.makedirs(self.output_dir, exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"{timestamp}_{self.label}_VehicleDynamics.csv"
        filepath = os.path.join(self.output_dir, filename)

        if not self.rows:
            print(f"  [WARN] No data for {self.label}")
            return None

        fieldnames = list(self.rows[0].keys())
        with open(filepath, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(self.rows)

        print(f"  [SAVE] {filename} ({len(self.rows)} frames)")
        return filepath


# ============================================================
# Simulation
# ============================================================

def spawn_npc_vehicles(world, num_npcs):
    """Spawn NPC vehicles with deterministic fixed controls (no traffic manager)."""
    bp_lib = world.get_blueprint_library()
    spawn_points = world.get_map().get_spawn_points()
    vehicles = []

    for i in range(num_npcs):
        bp = random.choice(bp_lib.filter('vehicle'))
        if bp.has_attribute('color'):
            color = random.choice(bp.get_attribute('color').recommended_values)
            bp.set_attribute('color', color)

        sp = spawn_points[i % len(spawn_points)]
        if i >= len(spawn_points):
            sp.location += carla.Location(
                x=random.uniform(-3, 3), y=random.uniform(-3, 3), z=0.2)

        vehicle = world.spawn_actor(bp, sp)
        ctrl = carla.VehicleControl()
        ctrl.throttle = 0.5 + 0.1 * (i % 3)
        ctrl.steer = 0.05 * (i % 3 - 1)
        vehicle.apply_control(ctrl)
        vehicles.append(vehicle)

    return vehicles


def run_one_simulation(client, label, snap_a=1000, snap_b=12000,
                       total_ticks=300, num_npcs=None):
    """
    Run one simulation and record ego vehicle trajectory.

    snap_a=1000, snap_b=12000 disables snapshot (never triggered since ticks < 1000)
    """
    print(f"\n{'='*60}")
    print(f"  RUN: {label}")
    snap_enabled = snap_a < 1000
    print(f"  Snapshot: {'ENABLED [' + str(snap_a) + ', ' + str(snap_b) + ')' if snap_enabled else 'DISABLED'}")
    print(f"  Ticks: {total_ticks}")
    print(f"{'='*60}")

    write_myab(snap_a, snap_b)
    time.sleep(0.3)

    if num_npcs is None:
        num_npcs = read_int_file(MYTOTAL_FILE, 2)

    print(f"  NPC vehicles: {num_npcs} (+ 1 EGO = {num_npcs + 1} total)")

    world = client.get_world()
    original_settings = world.get_settings()

    # Synchronous mode (no traffic manager needed - fixed controls)
    settings = world.get_settings()
    settings.synchronous_mode = True
    settings.fixed_delta_seconds = 0.05
    world.apply_settings(settings)

    vehicles = []
    recorder = TrajectoryRecorder(EXPERIMENT_DIR, label)

    try:
        # Spawn EGO with fixed controls (no traffic manager)
        bp = world.get_blueprint_library().filter('charger_2020')[0]
        ego = world.spawn_actor(bp, world.get_map().get_spawn_points()[-1])
        vehicles.append(ego)
        ego_ctrl = carla.VehicleControl()
        ego_ctrl.throttle = 0.6
        ego.apply_control(ego_ctrl)

        # Spawn NPCs with fixed controls
        npc_vehicles = spawn_npc_vehicles(world, num_npcs)
        vehicles.extend(npc_vehicles)
        print(f"  Vehicles spawned: {len(vehicles)}")

        # IMPORTANT: No settle ticks! Python cnt_tick and C++ myTickCount
        # must stay in sync. Both start at 0, both increment by 1 per world.tick().
        # The first world.tick() makes cnt_tick=1 and myTickCount=1.

        # Main loop
        cnt_tick = 0
        t_start = time.time()

        while cnt_tick < total_ticks:
            # Python-side snapshot skip (must match C++ side)
            if snap_enabled and cnt_tick == snap_a:
                print(f"  [SNAP] Python skip: tick {snap_a} → {snap_b}")
                cnt_tick = snap_b - 1
                if snap_b > total_ticks:
                    break

            t0 = time.perf_counter()
            world.tick()
            tick_time_ms = int((time.perf_counter() - t0) * 1000)

            cnt_tick += 1
            recorder.record(ego, cnt_tick, tick_time_ms)

            if cnt_tick % 50 == 0:
                elapsed = time.time() - t_start
                print(f"  ... tick {cnt_tick}/{total_ticks} ({elapsed:.1f}s)")

        elapsed = time.time() - t_start
        sim_time = total_ticks * 0.05
        print(f"  [DONE] {cnt_tick} ticks in {elapsed:.1f}s "
              f"(sim: {sim_time:.1f}s, ratio: {elapsed/sim_time:.1f}x)")

    except Exception as e:
        print(f"  [ERROR] {e}")
        import traceback
        traceback.print_exc()
        return None

    finally:
        csv_path = recorder.save()

        # Destroy all vehicles
        client.apply_batch([carla.command.DestroyActor(x) for x in vehicles])

        # CRITICAL: Tick a few times with sync mode still ON so C++ side
        # detects myVehicleNumber==0 and resets myTickCount to 0.
        # Without this, myTickCount accumulates across runs!
        print(f"  [RESET] Ticking to reset C++ myTickCount...")
        for _ in range(5):
            world.tick()
        print(f"  [RESET] Done. myTickCount should be 0 for next run.")

        world.apply_settings(original_settings)
        write_myab(1000, 12000)

    return csv_path


# ============================================================
# Analysis
# ============================================================

def load_trajectory(fp):
    rows = []
    with open(fp, 'r') as f:
        for row in csv.DictReader(f):
            rows.append({
                'frame': int(float(row['frame_count'])),
                'x': float(row['location_x']),
                'y': float(row['location_y']),
                'z': float(row['location_z']),
                'yaw': float(row['rotation_yaw']),
            })
    return rows


def analyze_trajectories(baseline_csv, snapshot_csv, snap_start, snap_end):
    print(f"\n{'='*60}")
    print(f"  ANALYSIS: Correctness Verification")
    print(f"{'='*60}")

    b_rows = load_trajectory(baseline_csv)
    s_rows = load_trajectory(snapshot_csv)

    b_by_frame = {r['frame']: r for r in b_rows}
    s_by_frame = {r['frame']: r for r in s_rows}

    common = sorted(set(b_by_frame.keys()) & set(s_by_frame.keys()))
    pre_frames = [f for f in common if f < snap_start]
    post_frames = [f for f in common if f >= snap_end]

    print(f"  Baseline frames: {len(b_rows)}, Snapshot frames: {len(s_rows)}")
    print(f"  Pre-snapshot  [0,{snap_start}): {len(pre_frames)} frames")
    print(f"  Post-snapshot [{snap_end},...]:  {len(post_frames)} frames")

    # Check snapshot gap in snapshot data
    s_frames_sorted = sorted(s_by_frame.keys())
    gaps = []
    for i in range(1, len(s_frames_sorted)):
        if s_frames_sorted[i] - s_frames_sorted[i-1] > 1:
            gaps.append((s_frames_sorted[i-1], s_frames_sorted[i]))
    if gaps:
        print(f"  Snapshot gaps detected: {gaps}")
    else:
        print(f"  No gaps in snapshot data (seems snapshot didn't trigger on Python side)")

    results = {}

    for region_name, frames_list in [("PRE-SNAPSHOT", pre_frames),
                                      ("POST-SNAPSHOT (CRITICAL)", post_frames)]:
        if not frames_list:
            print(f"\n  {region_name}: No frames to compare")
            continue

        diffs = {'x': [], 'y': [], 'z': [], 'yaw': [], 'euclidean': []}
        for f in frames_list:
            b, s = b_by_frame[f], s_by_frame[f]
            diffs['x'].append(abs(b['x'] - s['x']))
            diffs['y'].append(abs(b['y'] - s['y']))
            diffs['z'].append(abs(b['z'] - s['z']))
            diffs['yaw'].append(abs(b['yaw'] - s['yaw']))
            diffs['euclidean'].append(
                np.sqrt((b['x']-s['x'])**2 + (b['y']-s['y'])**2 + (b['z']-s['z'])**2))

        metrics = {}
        for col in ['x', 'y', 'z', 'yaw', 'euclidean']:
            arr = np.array(diffs[col])
            metrics[col] = {
                'max': float(arr.max()),
                'rmse': float(np.sqrt(np.mean(arr**2))),
                'mean': float(np.mean(arr)),
            }

        all_zero = all(metrics[c]['max'] < 1e-6 for c in ['x', 'y', 'z', 'yaw'])

        print(f"\n  {region_name}:")
        for c in ['x', 'y', 'z', 'yaw']:
            m = metrics[c]
            print(f"    {c:4s}: RMSE={m['rmse']:.2e}, Max={m['max']:.2e}  "
                  f"{'✅' if m['max'] < 1e-6 else '❌'}")
        print(f"    euclidean: RMSE={metrics['euclidean']['rmse']:.2e}, "
              f"Max={metrics['euclidean']['max']:.2e}")
        if all_zero:
            print(f"    ✅ ALL IDENTICAL")
        else:
            print(f"    ❌ DEVIATIONS DETECTED")

        metrics['all_identical'] = all_zero
        results[region_name] = metrics

    # Verdict
    print(f"\n{'='*60}")
    post_ok = results.get('POST-SNAPSHOT (CRITICAL)', {}).get('all_identical', False)
    pre_ok = results.get('PRE-SNAPSHOT', {}).get('all_identical', False)
    if post_ok and pre_ok:
        print(f"  VERDICT: ✅ SNAPSHOT IS LOSSLESS (CORRECTNESS CONFIRMED)")
    elif post_ok:
        print(f"  VERDICT: ✅ POST-SNAPSHOT TRAJECTORIES MATCH")
    else:
        print(f"  VERDICT: ❌ DEVIATIONS DETECTED — needs investigation")
    print(f"{'='*60}\n")

    return results


# ============================================================
# Main
# ============================================================

def main():
    parser = argparse.ArgumentParser(description="Snapshot Correctness Experiment")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=2000)
    parser.add_argument("--snap-start", type=int, default=100)
    parser.add_argument("--snap-end", type=int, default=200)
    parser.add_argument("--ticks", type=int, default=300)
    parser.add_argument("--skip-recording", action="store_true",
                        help="Skip recording run")
    parser.add_argument("--analyze-only", action="store_true",
                        help="Only run analysis on existing experiment data")
    args = parser.parse_args()

    os.makedirs(EXPERIMENT_DIR, exist_ok=True)
    random.seed(10)
    np.random.seed(10)

    total_vehicles = get_total_vehicles()
    print(f"Total vehicles (NPCs+EGO): {total_vehicles}")

    client = carla.Client(args.host, args.port)
    client.set_timeout(60.0)
    print(f"Connected to CARLA at {args.host}:{args.port}")

    if args.analyze_only:
        baselines = sorted(glob.glob(os.path.join(EXPERIMENT_DIR, "BASELINE_*.csv")))
        snapshots = sorted(glob.glob(os.path.join(EXPERIMENT_DIR, "SNAPSHOT_*.csv")))
        if baselines and snapshots:
            analyze_trajectories(baselines[-1], snapshots[-1],
                                 args.snap_start, args.snap_end)
        else:
            print("No experiment data found.")
        return 0

    # ---- Phase 1: Recording Run ----
    if not args.skip_recording:
        print(f"\n{'#'*60}")
        print(f"# PHASE 1: RECORDING RUN")
        print(f"{'#'*60}")

        backup_hardcoded_csvs(total_vehicles)

        # Record timestamp before run to find new files
        t_before = time.time()

        rec_csv = run_one_simulation(client, "RECORDING", 1000, 12000, args.ticks)
        if not rec_csv:
            print("[FATAL] Recording run failed!"); return 1

        time.sleep(1.0)  # Wait for C++ file writes

        # Find UWheeledVehicle4W CSVs created after our run started
        all_uw = glob.glob(os.path.join(DATA_DIR, "*_UWheeledVehicle4W_*.csv")) + \
                 glob.glob(os.path.join(DATA_DIR, "*_UWheeledVehicle4W.csv"))
        new_uwheel = [f for f in all_uw if os.path.getmtime(f) > t_before]
        new_uwheel.sort(key=os.path.getmtime)

        print(f"\n  New UWheeledVehicle4W CSVs: {len(new_uwheel)}")
        for f in new_uwheel:
            print(f"    {os.path.basename(f)} ({os.path.getsize(f)} bytes)")

        if not new_uwheel:
            print("[FATAL] No UWheeledVehicle4W CSVs generated!")
            print("The C++ VehicleDynamicsSaver may not be saving files.")
            print("Check UE4 Editor output log for 'jxy:' messages.")
            return 1

        for f in new_uwheel:
            dest = os.path.join(EXPERIMENT_DIR, "RECORDING_" + os.path.basename(f))
            shutil.copy2(f, dest)
        shutil.copy2(rec_csv, os.path.join(EXPERIMENT_DIR, "RECORDING_" + os.path.basename(rec_csv)))

        # ---- Phase 2: Copy to hardcoded paths AND update myReader.txt ----
        print(f"\n{'#'*60}")
        print(f"# PHASE 2: COPY TO HARDCODED PATHS + UPDATE myReader.txt")
        print(f"{'#'*60}")
        copy_csvs_for_replay(total_vehicles, new_uwheel)

        # Also write paths to myReader.txt for the new config-file-based loader
        reader_config = os.path.join(CONFIG_DIR, "myReader.txt")
        with open(reader_config, 'w') as f:
            for csv_path in new_uwheel:
                f.write(csv_path + "\n")
        print(f"  [CONFIG] myReader.txt updated with {len(new_uwheel)} paths")
        for p in new_uwheel:
            print(f"    {p}")
        time.sleep(0.3)

    # ---- Phase 3: Baseline Run ----
    print(f"\n{'#'*60}")
    print(f"# PHASE 3: BASELINE RUN")
    print(f"{'#'*60}")
    baseline_csv = run_one_simulation(client, "BASELINE", 1000, 12000, args.ticks)
    if not baseline_csv:
        print("[FATAL] Baseline run failed!"); return 1

    # ---- Phase 4: Snapshot Run ----
    print(f"\n{'#'*60}")
    print(f"# PHASE 4: SNAPSHOT RUN [{args.snap_start}, {args.snap_end})")
    print(f"{'#'*60}")
    snapshot_csv = run_one_simulation(client, "SNAPSHOT",
                                      args.snap_start, args.snap_end, args.ticks)
    if not snapshot_csv:
        print("[FATAL] Snapshot run failed!"); return 1

    # ---- Phase 5: Analysis ----
    print(f"\n{'#'*60}")
    print(f"# PHASE 5: ANALYSIS")
    print(f"{'#'*60}")
    results = analyze_trajectories(baseline_csv, snapshot_csv,
                                   args.snap_start, args.snap_end)

    # Save summary
    summary_path = os.path.join(EXPERIMENT_DIR, "correctness_results.txt")
    with open(summary_path, 'w') as f:
        f.write(f"Snapshot Correctness Experiment\n")
        f.write(f"===============================\n")
        f.write(f"Date: {datetime.now()}\n")
        f.write(f"Baseline: {baseline_csv}\n")
        f.write(f"Snapshot: {snapshot_csv}\n")
        f.write(f"Snapshot window: [{args.snap_start}, {args.snap_end})\n\n")
        for region_name, metrics in results.items():
            f.write(f"{region_name}:\n")
            for col in ['x', 'y', 'z', 'yaw', 'euclidean']:
                if col in metrics:
                    m = metrics[col]
                    f.write(f"  {col}: RMSE={m['rmse']:.6e}, Max={m['max']:.6e}\n")
            f.write(f"  Identical: {metrics.get('all_identical', 'N/A')}\n\n")

    print(f"Summary saved to: {summary_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
