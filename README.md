# HPC Galaxy Simulation Monitor (hpc-galaxy-sim)

Dự án thiết kế hệ thống và mã nguồn mô phỏng **Sự hình thành và tiến hóa của Thiên hà (Galaxy Formation & Evolution)** song song hóa bằng thư viện **MPI (Message Passing Interface)** tích hợp giao diện giám sát thời gian thực (Web Dashboard).

Học phần: **Tính toán hiệu năng cao (High Performance Computing)**  
Học viên thực hiện: **Phạm Quốc Nghị (MSHV: M3725006)**  
Trường Đại học Công nghệ Thông tin và Truyền thông, Đại học Cần Thơ.

---

## 🚀 Giới thiệu hệ thống

Hệ thống được thiết kế theo mô hình 3 lớp (3-tier architecture):
1. **Simulation Core (C/C++ MPI)**: Mã nguồn tính toán lực hấp dẫn Newtonian trực tiếp $O(N^2)$ có làm mềm lực $\epsilon$, được song song hóa bằng thư viện MPI sử dụng giao tiếp tập thể `MPI_Allgatherv`.
2. **Back-End (Python - FastAPI Modular)**: API điều phối và monitor được tổ chức theo cấu trúc module chuyên nghiệp. Tự động sinh `hostfile` cho môi trường multi-node, quản lý tiến trình con `mpirun`, thu thập log stdout thời gian thực từ rank 0, đo tải CPU thực tế qua `psutil` và đo lưu lượng mạng trao đổi dữ liệu.
3. **Front-End (HTML/JS - WebGL Three.js & Chart.js)**: Dashboard kính mờ (glassmorphic dark space) tích hợp đồ thị hiệu năng Chart.js, bản đồ phân chia tiến trình MPI Ranks Mapping, ô cấu hình IP cluster và màn hình terminal dòng lệnh hiển thị log thời gian thực.

---

## 📂 Cấu trúc thư mục mã nguồn

```text
hpc-galaxy-sim/
├── backend/
│   ├── app/
│   │   ├── __init__.py
│   │   ├── main.py       # Khởi tạo FastAPI app, CORS & tích hợp routers
│   │   ├── config.py     # Quản lý đường dẫn tương đối (portable paths)
│   │   ├── routers/
│   │   │   ├── simulation.py # API: /run, /stop, /status
│   │   │   └── monitor.py    # API: /monitor
│   │   ├── schemas/
│   │   │   └── simulation.py # Lược đồ Pydantic RunConfig
│   │   └── services/
│   │       └── simulation.py # Background worker quản lý mpirun & stdout logs
│   ├── requirements.txt  # Các thư viện python phụ thuộc
│   └── run.py            # Kịch bản khởi chạy Uvicorn server (cổng 8000)
├── frontend/
│   └── index.html        # Web Dashboard giám sát và visualizer 3D Three.js
├── scripts/
│   ├── nbody_seq.c       # Mã nguồn C mô phỏng tuần tự
│   ├── nbody_mpi.c       # Mã nguồn C mô phỏng song song bằng MPI
│   └── generate_galaxy_data.py # Script sinh dữ liệu Keplerian của 2 thiên hà
├── data/                 # Thư mục chứa các tệp dữ liệu tọa độ hạt sinh ra
│   ├── galaxy_collision_10k.txt
│   └── galaxy_collision_20k.txt
├── .gitignore
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
pip install -r backend/requirements.txt
```

**Bước 2**: Khởi chạy Back-End FastAPI:
```bash
python backend/run.py
```
Server sẽ lắng nghe tại địa chỉ `http://localhost:8000`. Bạn có thể truy cập tài liệu Swagger UI của API tại `http://localhost:8000/docs`.

**Bước 3**: Mở giao diện Dashboard:
Mở trực tiếp file `frontend/index.html` bằng trình duyệt Web của bạn.

---

## 🖥 Tính năng Giao diện Web Dashboard

- **Demo Mode**: Chạy mô phỏng cục bộ ngay trong trình duyệt ở tốc độ 60 FPS (sử dụng restricted 3-body solver) cùng chế độ bay camera tự động **Fly-Through Mode**.
- **Cluster Network Settings**: Nhập cấu hình địa chỉ IP cho Master và 4 Nodes. Bản đồ phân phối rank sẽ cập nhật hiển thị IP tương ứng ngay lập tức.
- **MPI Ranks Mapping**: Phân tích và biểu diễn trực quan cách chia luồng tiến trình (`Rank 0` tới `Rank P-1`) lên các Node cụ thể dựa trên số lượng Processes và Compute Nodes bạn chọn.
- **Performance Analysis**: Biểu đồ Chart.js so sánh hai trục Y: Thời gian thực thi (Time) và Tốc độ tăng tốc (Speedup). Đồ thị hiển thị sẵn đường benchmark thực tế và đường speedup lý thuyết. Khi lượt chạy mô phỏng hoàn tất, đồ thị tự động chấm thêm một điểm xanh lá (**Your Run**) thể hiện trực quan kết quả chạy thực tế của bạn.
- **MPI Terminal Output**: Khung console giả lập terminal in ra stdout dòng lệnh mpirun từ backend theo thời gian thực (các bước chạy rank, thời gian chạy, speedup, và hiệu suất Parallel Efficiency).
