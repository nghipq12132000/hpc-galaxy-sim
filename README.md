# HPC Galaxy Simulation Monitor (hpc-galaxy-sim)

Dự án nghiên cứu **Sự hình thành và tiến hóa của Thiên hà (Galaxy Formation & Evolution)** song song hóa bằng thư viện **MPI (Message Passing Interface)** và tích hợp hệ thống giám sát thời gian thực (Web Dashboard).

Học phần: **Tính toán hiệu năng cao (High Performance Computing)**  
Học viên thực hiện: **Phạm Quốc Nghị (MSHV: M3725006)**  
Trường Đại học Công nghệ Thông tin và Truyền thông, Đại học Cần Thơ.

---

## 🚀 Giới thiệu dự án

Dự án này giải quyết bài toán mô phỏng N-Body va chạm hấp dẫn giữa hai thiên hà (galaxy collision/merger) như Milky Way và Andromeda trong vật lý thiên văn. Các tính năng chính bao gồm:
1. **Simulation Core (C++)**: Mã nguồn mô phỏng N-Body tính lực hấp dẫn Newtonian trực tiếp $O(N^2)$ có làm mềm lực $\epsilon$, được song song hóa bằng thư viện MPI sử dụng giao tiếp tập thể `MPI_Allgatherv`.
2. **Back-End (Python - FastAPI)**: API điều phối và monitor, tự động chạy lệnh `mpirun` với tiến trình MPI chỉ định (tối đa 16 processes phân bố trên 1 Master + 4 Nodes), đồng thời đo tải CPU các nodes thông qua `psutil` và đo lưu lượng mạng MPI.
3. **Front-End (HTML/JS - Three.js Dashboard)**: Giao diện Web 3D kính mờ (glassmorphic dark space) hiển thị thời gian thực hoạt cảnh vụ va chạm thiên hà và đo các chỉ số hiệu năng (CPU Master, CPU Node 1-4, Network traffic).

---

## 📂 Cấu trúc thư mục

*   `main.tex`: File mã nguồn LaTeX chính của báo cáo cuối kỳ.
*   `source/`: Mã nguồn giao tiếp Front-End (`index.html` tích hợp WebGL Three.js và REST API).
*   `backend/`: Mã nguồn điều phối Back-End (`app.py` viết bằng FastAPI).
*   `scripts/`: Bộ mã nguồn C++ MPI, kịch bản tạo dữ liệu Keplerian đĩa xoay và kịch bản vẽ đồ thị hiệu năng.
*   `images/`: Đồ thị hiệu năng (Execution Time, Speedup, Efficiency) và sơ đồ minh họa Barnes-Hut, MPI_Allgatherv.
*   `docs/`: Các tài liệu học tập, cẩm nang lý thuyết HPC và báo cáo dạng Markdown.
*   `references/`: Các bài báo khoa học chất lượng cao tải về từ arXiv phục vụ nghiên cứu.

---

## 🛠 Hướng dẫn cài đặt và chạy thử nghiệm

### 1. Khởi chạy mô phỏng MPI trực tiếp qua Dòng lệnh (WSL/Linux)
Biên dịch phiên bản tuần tự (C):
```bash
gcc -O3 scripts/nbody_seq.c -o scripts/nbody_seq -lm
```
Biên dịch phiên bản song song (MPI):
```bash
mpicc -O3 scripts/nbody_mpi.c -o scripts/nbody_mpi -lm
```
Tạo dữ liệu ban đầu cho 2 đĩa thiên hà va chạm (10k hoặc 20k hạt):
```bash
python scripts/generate_galaxy_data.py
```
Chạy mô phỏng song song trên 4 tiến trình với tập dữ liệu 10k hạt trong 100 bước:
```bash
mpirun -np 4 ./scripts/nbody_mpi ./Data/galaxy_collision_10k.txt 100 0.01
```

### 2. Khởi chạy Web Dashboard Giám sát (Cấu hình FE + BE)
**Bước 1**: Cài đặt các thư viện Python bổ trợ cho backend:
```bash
pip install fastapi uvicorn psutil pydantic
```
**Bước 2**: Khởi chạy Back-End FastAPI:
```bash
python backend/app.py
```
Server sẽ lắng nghe tại cổng `http://localhost:8000`.

**Bước 3**: Mở giao diện Front-End:
Mở trực tiếp file `source/index.html` bằng trình duyệt Web của bạn. 

*Tính năng giao diện:*
*   **Demo Mode**: Chạy mô phỏng cục bộ ngay trong trình duyệt ở tốc độ 60 FPS (sử dụng restricted 3-body solver).
*   **Live MPI Run**: Kết nối và điều khiển trực tiếp FastAPI backend. Cho phép tăng giảm số lượng Processes (1-16) và Nodes hoạt động (1-4). Nhấn nút **Run MPI** để BE tự động chạy lệnh `mpirun` song song và đồng bộ hóa frame chuyển động lên canvas 3D, đồng thời cập nhật tải trọng CPU của các node trên bảng điều khiển.
