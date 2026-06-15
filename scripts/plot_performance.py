import matplotlib.pyplot as plt
import numpy as np
import os

def main():
    # Experimental data
    p = np.array([1, 2, 4, 8, 16])
    t_seq = 320.50
    t_p = np.array([320.50, 164.20, 83.80, 43.10, 23.40])
    
    # Calculate metrics
    speedup = t_seq / t_p
    efficiency = (speedup / p) * 100  # in %
    
    dest_dir = r"G:\My Drive\Classroom\Tính toán hiệu năng cao\Báo cáo\Artifacts"
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)
        
    # Styling configurations
    plt.style.use('seaborn-v0_8-darkgrid' if 'seaborn-v0_8-darkgrid' in plt.style.available else 'default')
    plt.rcParams['font.family'] = 'sans-serif'
    plt.rcParams['font.sans-serif'] = ['DejaVu Sans', 'Arial', 'Helvetica']
    
    # --- 1. Plot Execution Time ---
    plt.figure(figsize=(8, 5))
    plt.plot(p, t_p, marker='o', color='#3b82f6', linewidth=2.5, markersize=8, label='Thời gian thực tế (T_p)')
    plt.title('THỜI GIAN THỰC THI (EXECUTION TIME)\nN = 10,000 hạt, steps = 100, dt = 0.01', fontsize=12, fontweight='bold', pad=15)
    plt.xlabel('Số lượng Processes (p)', fontsize=11, labelpad=10)
    plt.ylabel('Thời gian chạy (giây)', fontsize=11, labelpad=10)
    plt.xticks(p)
    plt.grid(True, linestyle='--', alpha=0.6)
    for i, txt in enumerate(t_p):
        plt.annotate(f"{txt:.2f}s", (p[i], t_p[i]), textcoords="offset points", xytext=(0,10), ha='center', fontweight='bold', color='#1e3a8a')
    plt.ylim(0, 360)
    plt.tight_layout()
    plt.savefig(os.path.join(dest_dir, "execution_time.png"), dpi=150)
    plt.close()
    
    # --- 2. Plot Speedup ---
    plt.figure(figsize=(8, 5))
    plt.plot(p, speedup, marker='s', color='#10b981', linewidth=2.5, markersize=8, label='Tốc độ tăng tốc thực tế (S_p)')
    plt.plot(p, p, linestyle='--', color='#ef4444', linewidth=1.5, label='Tốc độ tăng tốc lý tưởng (S_p = p)')
    plt.title('TỐC ĐỘ TĂNG TỐC (SPEEDUP)\nN = 10,000 hạt, steps = 100, dt = 0.01', fontsize=12, fontweight='bold', pad=15)
    plt.xlabel('Số lượng Processes (p)', fontsize=11, labelpad=10)
    plt.ylabel('Tỷ lệ tăng tốc (lần)', fontsize=11, labelpad=10)
    plt.xticks(p)
    plt.yticks(np.arange(0, 18, 2))
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend(loc='upper left', frameon=True, facecolor='white', framealpha=0.9)
    for i, txt in enumerate(speedup):
        plt.annotate(f"{txt:.2f}x", (p[i], speedup[i]), textcoords="offset points", xytext=(0,10), ha='center', fontweight='bold', color='#065f46')
    plt.tight_layout()
    plt.savefig(os.path.join(dest_dir, "speedup.png"), dpi=150)
    plt.close()
    
    # --- 3. Plot Parallel Efficiency ---
    plt.figure(figsize=(8, 5))
    plt.plot(p, efficiency, marker='^', color='#8b5cf6', linewidth=2.5, markersize=8, label='Hiệu suất song song (E_p)')
    plt.axhline(y=100, linestyle='--', color='#ef4444', linewidth=1.5, label='Hiệu suất lý tưởng (100%)')
    plt.title('HIỆU SUẤT SONG SONG (PARALLEL EFFICIENCY)\nN = 10,000 hạt, steps = 100, dt = 0.01', fontsize=12, fontweight='bold', pad=15)
    plt.xlabel('Số lượng Processes (p)', fontsize=11, labelpad=10)
    plt.ylabel('Hiệu suất (%)', fontsize=11, labelpad=10)
    plt.xticks(p)
    plt.ylim(0, 110)
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend(loc='lower left', frameon=True, facecolor='white', framealpha=0.9)
    for i, txt in enumerate(efficiency):
        plt.annotate(f"{txt:.1f}%", (p[i], efficiency[i]), textcoords="offset points", xytext=(0,-20), ha='center', fontweight='bold', color='#5b21b6')
    plt.tight_layout()
    plt.savefig(os.path.join(dest_dir, "efficiency.png"), dpi=150)
    plt.close()
    
    print("Performance charts generated and saved successfully!")

if __name__ == '__main__':
    main()
