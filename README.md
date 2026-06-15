# HPC Galaxy Simulation Monitor (hpc-galaxy-sim)

Dự án thiết kế hệ thống và mã nguồn mô phỏng **Sự hình thành và tiến hóa của Thiên hà (Galaxy Formation & Evolution)** song song hóa bằng thư viện **MPI (Message Passing Interface)** tích hợp giao diện giám sát thời gian thực (Web Dashboard).

Học phần: **Tính toán hiệu năng cao (High Performance Computing)**  
Học viên thực hiện: **Phạm Quốc Nghị (MSHV: M3725006)**  
Trường Đại học Công nghệ Thông tin và Truyền thông, Đại học Cần Thơ.

---

## 🚀 Giới thiệu hệ thống

Hệ thống được thiết kế theo mô hình 3 lớp (3-tier architecture):
1. **Simulation Core (C/C++ MPI)**: Mã nguồn tính toán lực hấp dẫn Newtonian trực tiếp $O(N^2)$ có làm mềm lực $\epsilon$, được song song hóa bằng thư viện MPI sử dụng giao tiếp tập thể `MPI_Allgatherv`.
2. **Back-End (Python - FastAPI)**: API điều phối và monitor, tự động chạy lệnh `mpirun` với tiến trình MPI chỉ định (tối đa 16 processes phân bố trên 1 Master + 4 Nodes), tự động sinh `hostfile` cho môi trường multi-node, đo tải CPU thực tế qua `psutil` và đo lưu lượng mạng trao đổi dữ liệu.
3. **Front-End (HTML/JS - WebGL Three.js)**: Dashboard kính mờ (glassmorphic dark space) hiển thị thời gian thực hoạt cảnh vụ va chạm thiên hà và đo các chỉ số hiệu năng (CPU Master, CPU Node 1-4, Network traffic).

---

## 📂 Cấu trúc thư mục mã nguồn

```text
hpc-galaxy-sim/
├── backend/
│   └── app.py            # FastAPI Server quản lý và điều phối mpirun
├── frontend/
│   └── index.html        # Web Dashboard giám sát và visualizer 3D Three.js
├── scripts/
│   ├── nbody_seq.c       # Mã nguồn C mô phỏng tuần tự
│   ├── nbody_mpi.c       # Mã nguồn C mô phỏng song song bằng MPI
│   └── generate_galaxy_data.py # Script sinh dữ liệu Keplerian của 2 thiên hà
├── data/                 # Thư mục chứa các tệp dữ liệu tọa độ hạt sinh ra
│   ├── galaxy_collision_10k.txt
│   └── galaxy_collision_20k.txt
└── README.md
```

---

## 🛠 Hướng dẫn cài đặt và chạy thử nghiệm

### 1. Sinh dữ liệu khởi tạo thiên hà
Chạy kịch bản Python để sinh ra đĩa thiên hà với số lượng hạt mong muốn (10k hoặc 20k):
```bash
python scripts/generate_galaxy_data.py
```
Các file tọa độ dạng `.txt` sẽ được tạo trực tiếp tại thư mục `data/` trong repo.

### 2. Biên dịch và chạy mô phỏng MPI trực tiếp (WSL / Linux Cluster)
Biên dịch phiên bản tuần tự:
```bash
gcc -O3 scripts/nbody_seq.c -o scripts/nbody_seq -lm
```
Biên dịch phiên bản song song:
```bash
mpicc -O3 scripts/nbody_mpi.c -o scripts/nbody_mpi -lm
```
Chạy thử nghiệm mô phỏng song song với 4 tiến trình (ví dụ 100 bước chạy):
```bash
mpirun -np 4 ./scripts/nbody_mpi ./data/galaxy_collision_10k.txt 100 0.01
```

### 3. Khởi chạy Web Dashboard Giám sát (Cấu hình FE + BE)

**Bước 1**: Cài đặt các thư viện Python bổ trợ cho backend:
```bash
pip install fastapi uvicorn psutil pydantic
```

**Bước 2**: Khởi chạy Back-End FastAPI:
```bash
python backend/app.py
```
Server sẽ lắng nghe tại địa chỉ `http://localhost:8000`.

**Bước 3**: Mở giao diện Dashboard:
Mở trực tiếp file `frontend/index.html` bằng trình duyệt Web của bạn.

*Tính năng giao diện:*
- **Demo Mode**: Chạy mô phỏng cục bộ ngay trong trình duyệt ở tốc độ 60 FPS (sử dụng restricted 3-body solver).
- **Live MPI Run**: Kết nối và điều khiển trực tiếp FastAPI backend. Cho phép tăng giảm số lượng Processes (1-16) và Nodes hoạt động (1-4). Nhấn nút **Run MPI** để BE tự động chạy lệnh `mpirun` song song và đồng bộ hóa frame chuyển động lên canvas 3D, đồng thời cập nhật tải trọng CPU của các node trên bảng điều khiển. (Khi chạy trên Windows không cài MPI, backend tự động chuyển sang chế độ giả lập để debug GUI).
