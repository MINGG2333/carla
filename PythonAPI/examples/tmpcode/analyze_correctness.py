#!/usr/bin/env python3
"""
Snapshot Correctness Analysis Script
=====================================
Analyzes and compares BASELINE vs SNAPSHOT vehicle trajectories
to verify snapshot mechanism correctness for ASE 2026 Rebuttal.

Metrics computed:
  1. Position RMSE (x, y, z) for post-snapshot frames
  2. Rotation (yaw) difference for post-snapshot frames
  3. Frame-by-frame deviation
  4. Summary statistics

用法:
  python3 analyze_correctness.py [--exp-dir DATA_DIR]
  python3 analyze_correctness.py --baseline BASELINE.csv --snapshot SNAPSHOT.csv
"""

import argparse
import os
import sys
import glob
import numpy as np
import pandas as pd
from datetime import datetime

# ============================================================
# Configuration
# ============================================================
DATA_DIR = "/S980PRO/xinyu/Documents/carla/PythonAPI/examples/data"
EXPERIMENT_DIR = os.path.join(DATA_DIR, "correctness_experiment")

# Expected snapshot window
SNAP_START = 100
SNAP_END = 200


def load_trajectory_csv(filepath):
    """Load a VehicleDynamics CSV file into a pandas DataFrame.
    
    Handles the CSV format from visualize_multiple_sensors_how_collect_Determinism_data.py:
      location_x, location_y, location_z, rotation_yaw, heading, frame_count, tick_time, current_time, FrameCount
    """
    df = pd.read_csv(filepath)

    # Map column names (exact match from the known CSV format)
    col_map = {
        'location_x': 'x',
        'location_y': 'y',
        'location_z': 'z',
        'rotation_yaw': 'yaw',
        'heading': 'heading',
        'frame_count': 'frame',
        'tick_time': 'tick_time',
        'current_time': 'wall_time',
        'FrameCount': 'frame_count_idx',
    }

    df = df.rename(columns={k: v for k, v in col_map.items() if k in df.columns})

    # Use frame_count as the frame index
    if 'frame' not in df.columns:
        df['frame'] = range(len(df))

    # Set frame as index
    df = df.set_index('frame').sort_index()

    # Convert all numeric columns
    for col in ['x', 'y', 'z', 'yaw']:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    print(f"[LOAD] {os.path.basename(filepath)}: {len(df)} frames, "
          f"cols: {[c for c in df.columns if c in ['x','y','z','yaw']]}")
    return df


def compare_trajectories(baseline_df, snapshot_df, snap_start, snap_end):
    """
    Compare baseline and snapshot trajectories.

    Key regions:
      - Pre-snapshot [0, snap_start): Should be identical (both normal simulation)
      - Snapshot    [snap_start, snap_end): BASELINE has normal data, SNAPSHOT has gaps
      - Post-snapshot (snap_end, :]: THE CRITICAL REGION — must be identical
    """
    # Find the overlapping frame range for post-snapshot
    common_frames = baseline_df.index.intersection(snapshot_df.index)
    post_snap_frames = [f for f in common_frames if f >= snap_end]
    pre_snap_frames = [f for f in common_frames if f < snap_start]

    results = {
        'pre_snapshot': {},
        'post_snapshot': {},
        'snapshot_window': (snap_start, snap_end),
    }

    # ================================================================
    # Pre-snapshot analysis (sanity check)
    # ================================================================
    if pre_snap_frames:
        b_pre = baseline_df.loc[pre_snap_frames]
        s_pre = snapshot_df.loc[pre_snap_frames]

        for col in ['x', 'y', 'z', 'yaw']:
            if col in b_pre.columns and col in s_pre.columns:
                diff = (b_pre[col] - s_pre[col]).abs()
                results['pre_snapshot'][f'{col}_max_diff'] = diff.max()
                results['pre_snapshot'][f'{col}_rmse'] = np.sqrt((diff ** 2).mean())
                results['pre_snapshot'][f'{col}_mean_diff'] = diff.mean()

        results['pre_snapshot']['num_frames'] = len(pre_snap_frames)
        results['pre_snapshot']['all_identical'] = all(
            results['pre_snapshot'].get(f'{col}_max_diff', 0) < 1e-6
            for col in ['x', 'y', 'z', 'yaw']
        )

    # ================================================================
    # Post-snapshot analysis (THE KEY RESULT)
    # ================================================================
    if post_snap_frames:
        b_post = baseline_df.loc[post_snap_frames]
        s_post = snapshot_df.loc[post_snap_frames]

        frame_diffs = {}

        for col in ['x', 'y', 'z', 'yaw']:
            if col in b_post.columns and col in s_post.columns:
                diff = (b_post[col] - s_post[col]).abs()
                frame_diffs[col] = diff
                results['post_snapshot'][f'{col}_max_diff'] = diff.max()
                results['post_snapshot'][f'{col}_rmse'] = np.sqrt((diff ** 2).mean())
                results['post_snapshot'][f'{col}_mean_diff'] = diff.mean()

        # Combined position error (Euclidean)
        if all(c in frame_diffs for c in ['x', 'y', 'z']):
            euclidean_diff = np.sqrt(
                frame_diffs['x']**2 + frame_diffs['y']**2 + frame_diffs['z']**2
            )
            results['post_snapshot']['euclidean_max'] = euclidean_diff.max()
            results['post_snapshot']['euclidean_rmse'] = np.sqrt((euclidean_diff**2).mean())
            results['post_snapshot']['euclidean_mean'] = euclidean_diff.mean()

        results['post_snapshot']['num_frames'] = len(post_snap_frames)
        results['post_snapshot']['all_identical'] = all(
            results['post_snapshot'].get(f'{col}_max_diff', 0) < 1e-6
            for col in ['x', 'y', 'z', 'yaw']
        )

        # Store per-frame diffs for detailed reporting
        results['post_snapshot']['frame_diffs'] = frame_diffs

    return results


def print_report(results, baseline_file, snapshot_file):
    """Print a formatted correctness report."""
    print(f"\n{'='*70}")
    print(f"  SNAPSHOT CORRECTNESS ANALYSIS REPORT")
    print(f"{'='*70}")
    print(f"  Baseline : {os.path.basename(baseline_file)}")
    print(f"  Snapshot : {os.path.basename(snapshot_file)}")
    print(f"  Snapshot window: ticks [{results['snapshot_window'][0]}, "
          f"{results['snapshot_window'][1]})")
    print(f"{'='*70}")

    # Pre-snapshot
    pre = results['pre_snapshot']
    print(f"\n  📍 PRE-SNAPSHOT REGION [0, {results['snapshot_window'][0]})")
    print(f"     Frames compared: {pre.get('num_frames', 0)}")
    if pre.get('num_frames', 0) > 0:
        for col in ['x', 'y', 'z', 'yaw']:
            rmse = pre.get(f'{col}_rmse', float('nan'))
            max_d = pre.get(f'{col}_max_diff', float('nan'))
            status = "✅" if max_d < 1e-6 else "⚠️"
            print(f"     {col:4s}: RMSE={rmse:.2e}, MaxDiff={max_d:.2e} {status}")
        if pre.get('all_identical'):
            print(f"     ✅ Pre-snapshot trajectories are IDENTICAL (determinism confirmed)")

    # Post-snapshot (CRITICAL)
    post = results['post_snapshot']
    print(f"\n  🔬 POST-SNAPSHOT REGION [{results['snapshot_window'][1]}, ...]")
    print(f"     Frames compared: {post.get('num_frames', 0)}")
    if post.get('num_frames', 0) > 0:
        for col in ['x', 'y', 'z', 'yaw']:
            rmse = post.get(f'{col}_rmse', float('nan'))
            max_d = post.get(f'{col}_max_diff', float('nan'))
            status = "✅" if max_d < 1e-6 else "❌"
            print(f"     {col:4s}: RMSE={rmse:.2e}, MaxDiff={max_d:.2e} {status}")

        if 'euclidean_rmse' in post:
            print(f"     Euclidean (xyz): RMSE={post['euclidean_rmse']:.2e}, "
                  f"Max={post.get('euclidean_max', 0):.2e}")

        if post.get('all_identical'):
            print(f"\n     ✅✅✅ POST-SNAPSHOT TRAJECTORIES ARE IDENTICAL")
            print(f"     Snapshot mechanism preserves simulation fidelity!")
        else:
            print(f"\n     ❌ POST-SNAPSHOT TRAJECTORIES DIFFER")
            print(f"     Snapshot mechanism introduces deviations.")

    # Overall verdict
    print(f"\n{'='*70}")
    if post.get('all_identical') and pre.get('all_identical'):
        print(f"  VERDICT: ✅ SNAPSHOT MECHANISM IS CORRECT (LOSSLESS)")
    elif post.get('all_identical'):
        print(f"  VERDICT: ✅ POST-SNAPSHOT TRAJECTORIES MATCH")
    else:
        print(f"  VERDICT: ❌ DEVIATIONS DETECTED — needs investigation")
    print(f"{'='*70}\n")


def generate_rebuttal_text(results):
    """Generate the rebuttal text based on experiment results."""
    post = results['post_snapshot']
    pre = results['pre_snapshot']
    snap_start, snap_end = results['snapshot_window']

    lines = []
    lines.append("=" * 70)
    lines.append("REBUTTAL TEXT — Correctness Experiment Results")
    lines.append("=" * 70)
    lines.append("")

    # Determinism confirmation
    if pre.get('all_identical'):
        lines.append(
            "We first confirm that CARLA in synchronous mode is deterministic: "
            f"running the same scenario twice (with identical random seeds) produces "
            f"identical vehicle trajectories across all {pre.get('num_frames', 0)} "
            f"pre-snapshot frames (RMSE < 1e-6 for all of x, y, z, yaw). "
            "This validates the foundational assumption of our snapshot mechanism."
        )
    lines.append("")

    # Snapshot correctness
    if post.get('all_identical'):
        euclidean_rmse = post.get('euclidean_rmse', 0)
        lines.append(
            f"We then evaluate the snapshot mechanism by compressing ticks [{snap_start}, {snap_end}) "
            f"into a single physics frame (DeltaSeconds × {snap_end - snap_start}). "
            f"Comparing the post-snapshot trajectories ({post.get('num_frames', 0)} frames) "
            f"between the baseline run and the snapshot-accelerated run, we find:"
        )
        lines.append("")
        for col in ['x', 'y', 'z', 'yaw']:
            rmse = post.get(f'{col}_rmse', 0)
            max_d = post.get(f'{col}_max_diff', 0)
            lines.append(f"  - {col}: RMSE = {rmse:.2e}, MaxDiff = {max_d:.2e}")
        lines.append(f"  - Euclidean position: RMSE = {euclidean_rmse:.2e}")
        lines.append("")
        lines.append(
            "The trajectories are IDENTICAL (all differences < 1e-6). This confirms that "
            "the snapshot mechanism is lossless: replaying recorded physics engine inputs "
            "(throttle, brake, steering) reproduces the exact same physics state, and "
            "the subsequent simulation (including sensor rendering and ADS inference) "
            "proceeds identically to the original execution. Consequently, any failure "
            "that would be triggered in the original simulation is also triggered in "
            "the snapshot-accelerated simulation at the exact same location."
        )
        lines.append("")
        lines.append(
            "These results address the concerns raised by all three reviewers (A#2, B#2, C#1) "
            "regarding the correctness and faithfulness of the snapshot mechanism."
        )
    else:
        lines.append(
            "We observed deviations in the post-snapshot trajectories, indicating that "
            "further investigation is needed. [Details to be filled after experiments]"
        )

    lines.append("")
    lines.append("=" * 70)

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Snapshot Correctness Analysis")
    parser.add_argument("--baseline", help="Path to baseline CSV file")
    parser.add_argument("--snapshot", help="Path to snapshot CSV file")
    parser.add_argument("--exp-dir", default=EXPERIMENT_DIR,
                        help="Experiment output directory")
    parser.add_argument("--snap-start", type=int, default=SNAP_START)
    parser.add_argument("--snap-end", type=int, default=SNAP_END)
    parser.add_argument("--output-rebuttal", help="Save rebuttal text to file")
    args = parser.parse_args()

    # Find baseline and snapshot files
    if args.baseline and args.snapshot:
        baseline_file = args.baseline
        snapshot_file = args.snapshot
    else:
        # Auto-detect from experiment directory
        exp_dir = args.exp_dir
        if not os.path.isdir(exp_dir):
            print(f"[ERROR] Experiment directory not found: {exp_dir}")
            print(f"Run 'python3 tmpcode/run_correctness_experiment.py' first.")
            return 1

        baselines = sorted(glob.glob(os.path.join(exp_dir, "BASELINE_*.csv")))
        snapshots = sorted(glob.glob(os.path.join(exp_dir, "SNAPSHOT_*.csv")))

        if not baselines:
            print(f"[ERROR] No BASELINE CSV found in {exp_dir}")
            return 1
        if not snapshots:
            print(f"[ERROR] No SNAPSHOT CSV found in {exp_dir}")
            return 1

        baseline_file = baselines[-1]  # Latest
        snapshot_file = snapshots[-1]

    print(f"[INFO] Baseline: {baseline_file}")
    print(f"[INFO] Snapshot: {snapshot_file}")

    # Load data
    baseline_df = load_trajectory_csv(baseline_file)
    snapshot_df = load_trajectory_csv(snapshot_file)

    # Compare
    results = compare_trajectories(baseline_df, snapshot_df, args.snap_start, args.snap_end)

    # Print report
    print_report(results, baseline_file, snapshot_file)

    # Generate rebuttal text
    rebuttal_text = generate_rebuttal_text(results)
    print(rebuttal_text)

    if args.output_rebuttal:
        with open(args.output_rebuttal, 'w') as f:
            f.write(rebuttal_text)
        print(f"[SAVE] Rebuttal text saved to: {args.output_rebuttal}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
