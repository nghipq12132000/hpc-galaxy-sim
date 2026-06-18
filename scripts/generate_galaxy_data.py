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
    # Scale length of the exponential disk
    h = 8.0
    for i in range(1, num_stars + 1):
        # Sample radius r from exponential disk density profile p(r) ~ r * exp(-r/h)
        # using a Gamma distribution of shape=2
        r = np.random.gamma(2, h)
        while r < min_r or r > max_r:
            r = np.random.gamma(2, h)
            
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
        
        # Velocity dispersion (approx 15% of circular orbital velocity) representing thermal motion
        v_disp = 0.15 * v_orbit
        
        # Orbital velocity vector (perpendicular to radial position vector)
        # For a clockwise or counterclockwise disk rotation
        direction = 1 if galaxy_id == 1 else -1
        vx_rel = -direction * v_orbit * np.sin(theta) + np.random.normal(0, v_disp)
        vy_rel = direction * v_orbit * np.cos(theta) + np.random.normal(0, v_disp)
        vz_rel = np.random.normal(0, v_disp * 0.5)
        
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
    
    # Parameters
    g1_pos = np.array([-80.0, 0.0, 0.0])
    g1_vel = np.array([2.0, 0.4, 0.0]) # has minor y component for non-head-on collision
    g1_mass = 22000.0
    
    g2_pos = np.array([80.0, 0.0, 0.0])
    g2_vel = np.array([-2.0, -0.4, 0.0])
    g2_mass = 22000.0
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_dir = os.path.dirname(script_dir)
    dest_dir = os.path.join(repo_dir, "data")
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)
        
    # Check if a custom particle count was passed as command line argument
    if len(sys.argv) > 1:
        try:
            N = int(sys.argv[1])
            if N < 100 or N > 100000000:
                print(f"Error: Particle count must be between 100 and 100,000,000. Got {N}")
                sys.exit(1)
            
            stars_per_galaxy = (N // 2) - 1
            g1_data = generate_galaxy(g1_pos, g1_vel, g1_mass, stars_per_galaxy, 5.0, 32.0, 1)
            g2_data = generate_galaxy(g2_pos, g2_vel, g2_mass, stars_per_galaxy, 5.0, 32.0, 2)
            data = np.vstack((g1_data, g2_data))
            
            # Format output name
            if N >= 1000000:
                suffix = f"{N//1000000}m" if N % 1000000 == 0 else f"{N/1000000:.1f}m"
            else:
                suffix = f"{N//1000}k" if N % 1000 == 0 else f"{N}"
            out_file = os.path.join(dest_dir, f"galaxy_collision_{suffix}.txt")
            np.savetxt(out_file, data, fmt="%.6f %.6f %.6f %.6f %.6f %.6f %.6f")
            print(f"Generated {N} particle data at {out_file}")
            return
        except ValueError:
            print("Error: Invalid particle count argument. Must be an integer.")
            sys.exit(1)
            
    # Default behavior: generate both 10k and 20k data
    N_10k = 10000
    N_20k = 20000
    
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
