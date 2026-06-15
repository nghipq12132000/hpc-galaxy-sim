import numpy as np
import os
import sys

def generate_galaxy(center_pos, center_vel, center_mass, num_stars, min_r, max_r, galaxy_id):
    # Allocate arrays
    # 7 columns: x y z vx vy vz mass
    data = np.zeros((num_stars + 1, 7))
    
    # 1. Central Supermassive Black Hole (particle 0)
    data[0, 0:3] = center_pos
    data[0, 3:6] = center_vel
    data[0, 6] = center_mass
    
    # Gravitational constant in our scaled units
    G = 1.0
    
    # 2. Generate stars
    for i in range(1, num_stars + 1):
        # Semi-random distribution in a disc
        # R follows a power law or uniform distribution
        r = np.random.uniform(min_r, max_r)
        theta = np.random.uniform(0, 2 * np.pi)
        
        # Positions relative to center
        x_rel = r * np.cos(theta)
        y_rel = r * np.sin(theta)
        z_rel = np.random.normal(0, r * 0.05) # thin disc with a bit of vertical dispersion
        
        # Absolute positions
        x = center_pos[0] + x_rel
        y = center_pos[1] + y_rel
        z = center_pos[2] + z_rel
        
        # Orbital velocity magnitude v = sqrt(G * M_center / r)
        v_orbit = np.sqrt(G * center_mass / r)
        
        # Orbital velocity vector (perpendicular to radial position vector)
        # For a clockwise or counterclockwise disk rotation
        # Let's make galaxy 1 and galaxy 2 rotate in opposite directions for visual dynamics
        direction = 1 if galaxy_id == 1 else -1
        vx_rel = -direction * v_orbit * np.sin(theta)
        vy_rel = direction * v_orbit * np.cos(theta)
        vz_rel = 0.0
        
        # Absolute velocities
        vx = center_vel[0] + vx_rel
        vy = center_vel[1] + vy_rel
        vz = center_vel[2] + vz_rel
        
        # Mass of star (small compared to center black hole)
        mass = np.random.uniform(0.1, 1.0)
        
        data[i] = [x, y, z, vx, vy, vz, mass]
        
    return data

def main():
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
    np.random.seed(42)
    
    # Total particles N = 10,000 (each galaxy has 5000 particles: 1 black hole + 4999 stars)
    N_10k = 10000
    N_20k = 20000
    
    # Parameters
    g1_pos = np.array([-50.0, 0.0, 0.0])
    g1_vel = np.array([3.2, 0.6, 0.0]) # has minor y component for non-head-on collision
    g1_mass = 22000.0
    
    g2_pos = np.array([50.0, 0.0, 0.0])
    g2_vel = np.array([-3.2, -0.6, 0.0])
    g2_mass = 22000.0
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_dir = os.path.dirname(script_dir)
    dest_dir = os.path.join(repo_dir, "data")
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)
        
    # Generate 10k data
    stars_per_galaxy_10k = (N_10k // 2) - 1
    g1_data = generate_galaxy(g1_pos, g1_vel, g1_mass, stars_per_galaxy_10k, 5.0, 32.0, 1)
    g2_data = generate_galaxy(g2_pos, g2_vel, g2_mass, stars_per_galaxy_10k, 5.0, 32.0, 2)
    data_10k = np.vstack((g1_data, g2_data))
    
    out_10k = os.path.join(dest_dir, "galaxy_collision_10k.txt")
    np.savetxt(out_10k, data_10k, fmt="%.6f %.6f %.6f %.6f %.6f %.6f %.6f")
    print(f"Generated 10k particle data at {out_10k}")
    
    # Generate 20k data
    stars_per_galaxy_20k = (N_20k // 2) - 1
    g1_data_20k = generate_galaxy(g1_pos, g1_vel, g1_mass, stars_per_galaxy_20k, 5.0, 32.0, 1)
    g2_data_20k = generate_galaxy(g2_pos, g2_vel, g2_mass, stars_per_galaxy_20k, 5.0, 32.0, 2)
    data_20k = np.vstack((g1_data_20k, g2_data_20k))
    
    out_20k = os.path.join(dest_dir, "galaxy_collision_20k.txt")
    np.savetxt(out_20k, data_20k, fmt="%.6f %.6f %.6f %.6f %.6f %.6f %.6f")
    print(f"Generated 20k particle data at {out_20k}")

if __name__ == '__main__':
    main()
