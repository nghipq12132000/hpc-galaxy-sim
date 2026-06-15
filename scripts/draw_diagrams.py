import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
import os
import sys

# Reconfigure stdout to handle Vietnamese characters
sys.stdout.reconfigure(encoding='utf-8')

def draw_barnes_hut(dest_path):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 5.5), gridspec_kw={'width_ratios': [1, 1.1]})
    
    # --- Left Panel: 2D Quadtree Space Partition ---
    ax1.set_xlim(0, 10)
    ax1.set_ylim(0, 10)
    ax1.set_aspect('equal')
    ax1.set_title("Phân hoạch không gian 2D (Quadtree)", fontsize=11, fontweight='bold', pad=10)
    
    # Draw boundary of root node
    ax1.add_patch(patches.Rectangle((0, 0), 10, 10, fill=False, edgecolor='black', linewidth=2))
    
    # First level split (x=5, y=5)
    ax1.axvline(x=5, color='gray', linestyle='--', linewidth=1.5)
    ax1.axhline(y=5, color='gray', linestyle='--', linewidth=1.5)
    
    # Second level split in NW quadrant (x=[0,5], y=[5,10]) -> split at x=2.5, y=7.5
    ax1.plot([2.5, 2.5], [5, 10], color='gray', linestyle=':', linewidth=1.2)
    ax1.plot([0, 5], [7.5, 7.5], color='gray', linestyle=':', linewidth=1.2)
    
    # Second level split in SE quadrant (x=[5,10], y=[0,5]) -> split at x=7.5, y=2.5
    ax1.plot([7.5, 7.5], [0, 5], color='gray', linestyle=':', linewidth=1.2)
    ax1.plot([5, 10], [2.5, 2.5], color='gray', linestyle=':', linewidth=1.2)
    
    # Generate particles (points)
    particles = [
        (1.2, 8.5, 'A'), (3.8, 6.2, 'B'), # NW
        (7.5, 8.0, 'C'),                  # NE
        (2.0, 3.0, 'D'),                  # SW
        (6.1, 1.5, 'E'), (8.8, 4.0, 'F'), (9.2, 1.1, 'G') # SE
    ]
    
    for x, y, label in particles:
        ax1.plot(x, y, 'ro', markersize=7, markeredgecolor='black')
        ax1.text(x + 0.2, y + 0.2, label, fontsize=10, fontweight='bold')
        
    ax1.text(0.2, 9.5, "NW", fontsize=10, fontweight='bold', color='blue')
    ax1.text(5.2, 9.5, "NE", fontsize=10, fontweight='bold', color='blue')
    ax1.text(0.2, 4.5, "SW", fontsize=10, fontweight='bold', color='blue')
    ax1.text(5.2, 4.5, "SE", fontsize=10, fontweight='bold', color='blue')
    ax1.set_xticks([])
    ax1.set_yticks([])
    
    # --- Right Panel: Tree Hierarchy ---
    ax2.set_xlim(0, 10)
    ax2.set_ylim(0, 10)
    ax2.set_aspect('equal')
    ax2.set_title("Cấu trúc Cây tương ứng", fontsize=11, fontweight='bold', pad=10)
    ax2.axis('off')
    
    # Helper to draw node
    def draw_node(x, y, label, is_leaf=False):
        color = '#fecaca' if is_leaf else '#bfdbfe'
        edge = 'red' if is_leaf else 'blue'
        box = patches.FancyBboxPatch((x-0.4, y-0.3), 0.8, 0.6, boxstyle="round,pad=0.1", 
                                     facecolor=color, edgecolor=edge, linewidth=1.5)
        ax2.add_patch(box)
        ax2.text(x, y, label, ha='center', va='center', fontsize=9, fontweight='bold')
        
    # Draw tree edges
    def draw_edge(x1, y1, x2, y2):
        ax2.annotate("", xy=(x2, y2+0.3), xytext=(x1, y1-0.3),
                    arrowprops=dict(arrowstyle="->", color='gray', lw=1.2))
                    
    # Root node (Level 0)
    draw_node(5, 9, "Root")
    
    # Level 1 Nodes
    draw_node(2, 6, "NW")
    draw_node(4, 6, "NE (C)")
    draw_node(6, 6, "SW (D)")
    draw_node(8, 6, "SE")
    draw_edge(5, 9, 2, 6)
    draw_edge(5, 9, 4, 6)
    draw_edge(5, 9, 6, 6)
    draw_edge(5, 9, 8, 6)
    
    # Level 2 Nodes under NW
    draw_node(0.8, 3, "A", is_leaf=True)
    draw_node(2.0, 3, "B", is_leaf=True)
    draw_node(3.2, 3, "empty", is_leaf=True)
    draw_node(4.4, 3, "empty", is_leaf=True)
    draw_edge(2, 6, 0.8, 3)
    draw_edge(2, 6, 2.0, 3)
    draw_edge(2, 6, 3.2, 3)
    draw_edge(2, 6, 4.4, 3)
    
    # Level 2 Nodes under SE
    draw_node(5.6, 3, "empty", is_leaf=True)
    draw_node(6.8, 3, "E", is_leaf=True)
    draw_node(8.0, 3, "F", is_leaf=True)
    draw_node(9.2, 3, "G", is_leaf=True)
    draw_edge(8, 6, 5.6, 3)
    draw_edge(8, 6, 6.8, 3)
    draw_edge(8, 6, 8.0, 3)
    draw_edge(8, 6, 9.2, 3)
    
    plt.tight_layout()
    plt.savefig(dest_path, dpi=150)
    plt.close()
    print(f"Generated Barnes-Hut quadtree diagram at {dest_path}")

def draw_mpi_allgatherv(dest_path):
    fig, ax = plt.subplots(figsize=(10, 6.5))
    ax.set_xlim(0, 12)
    ax.set_ylim(-0.5, 9.5)
    ax.set_aspect('equal')
    ax.axis('off')
    ax.set_title("Cơ chế truyền thông MPI_Allgatherv", fontsize=12, fontweight='bold', pad=15)
    
    # Process rows
    ranks = 4
    colors = ['#f87171', '#4ade80', '#60a5fa', '#facc15'] # Red, Green, Blue, Yellow
    block_sizes = [3, 2, 4, 2] # sum = 11
    
    # Draw "BEFORE" state (y=6..8)
    ax.text(0.2, 8.2, "Trước khi Allgatherv (Dữ liệu cục bộ trên mỗi Rank)", fontsize=10, fontweight='bold', color='#1e3a8a')
    for r in range(ranks):
        y = 7.2 - r * 0.7
        ax.text(0.5, y + 0.15, f"Rank {r}:", fontsize=9, fontweight='bold')
        
        # Local buffer block
        rect_local = patches.Rectangle((2.0 + (r * 1.5), y), block_sizes[r]*0.3, 0.4, 
                                      facecolor=colors[r], edgecolor='black', linewidth=1)
        ax.add_patch(rect_local)
        ax.text(2.0 + (r * 1.5) + block_sizes[r]*0.15, y + 0.12, f"data{r}", 
                ha='center', va='center', fontsize=8, fontweight='bold')
        
        # Empty placeholder line representing local buffer size
        ax.plot([2.0, 7.0], [y, y], color='gray', linestyle=':', alpha=0.3, zorder=0)

    # Draw arrow down (y=3..4)
    ax.annotate("MPI_Allgatherv", xy=(5.5, 2.5), xytext=(5.5, 4.3),
                arrowprops=dict(facecolor='#8b5cf6', edgecolor='black', shrink=0.05, width=15, headwidth=25),
                ha='center', va='center', fontsize=11, fontweight='bold', color='#5b21b6')
                
    # Draw "AFTER" state (y=0..2)
    ax.text(0.2, 1.8, "Sau khi Allgatherv (Dữ liệu được đồng bộ toàn cục trên tất cả các Ranks)", fontsize=10, fontweight='bold', color='#10b981')
    
    for r in range(ranks):
        y = 1.0 - r * 0.45
        ax.text(0.5, y + 0.1, f"Rank {r}:", fontsize=9, fontweight='bold')
        
        # Draw the full combined array on each rank
        start_x = 2.0
        for i in range(ranks):
            w = block_sizes[i] * 0.4
            rect_part = patches.Rectangle((start_x, y), w, 0.3, 
                                          facecolor=colors[i], edgecolor='black', linewidth=1)
            ax.add_patch(rect_part)
            ax.text(start_x + w/2, y + 0.1, f"data{i}", 
                    ha='center', va='center', fontsize=8, fontweight='bold')
            start_x += w
            
        # Draw bracket indicating total gathered array
        if r == 0:
            ax.annotate("", xy=(2.0, y + 0.4), xytext=(start_x, y + 0.4),
                        arrowprops=dict(arrowstyle="|-|", color='black', lw=1.2))
            ax.text((2.0 + start_x)/2, y + 0.55, f"Tổng mảng toàn cục (N = {sum(block_sizes)} hạt)", 
                    ha='center', va='center', fontsize=9, fontweight='bold', color='#1e293b')

    plt.tight_layout()
    plt.savefig(dest_path, dpi=150)
    plt.close()
    print(f"Generated MPI_Allgatherv diagram at {dest_path}")

def main():
    report_dir = r"G:\My Drive\Classroom\Tính toán hiệu năng cao\Báo cáo\Baocao"
    if not os.path.exists(report_dir):
        os.makedirs(report_dir)
        
    draw_barnes_hut(os.path.join(report_dir, "barnes_hut.png"))
    draw_mpi_allgatherv(os.path.join(report_dir, "mpi_allgatherv.png"))
    print("All diagrams drawn and saved successfully in report directory!")

if __name__ == '__main__':
    main()
