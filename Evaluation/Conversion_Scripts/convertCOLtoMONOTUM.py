import numpy as np
from scipy.spatial.transform import Rotation as R
import argparse

N_FPS = 5

def quat_to_rot(qw, qx, qy, qz):
    rot = R.from_quat([qx, qy, qz, qw])
    return rot.as_matrix()

parser = argparse.ArgumentParser(
                    prog='Colmap to TUM',
                    description='Convert COLMAP format to a MONOTUM Orb-Slam output format',
                    epilog='FoobarFizzBuzz')
parser.add_argument('filename')
args = parser.parse_args()

increment = 1/N_FPS
timestamp = 0
with open(args.filename) as fin, open('output.txt', 'w') as fout:
    for line in fin:
        if line.strip().startswith('#'):
            continue
        parts = line.split()

        timestamp += increment
        qw, qx, qy, qz = map(float, parts[1:5])
        tx, ty, tz = map(float, parts[5:8])

        R_mat = quat_to_rot(qw, qx, qy, qz)
        cam_ctr = -R_mat.T.dot(np.array([tx, ty, tz]))

        fout.write(
            f"{timestamp:.6f} "
            f"{cam_ctr[0]:.6f} {cam_ctr[1]:.6f} {cam_ctr[2]:.6f} "
            f"{qx:.6f} {qy:.6f} {qz:.6f} {qw:.6f}\n"
        )

print("Wrote trajectory in TUM format to output.txt")
