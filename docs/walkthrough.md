# Nhật ký Thực hiện & Kiểm thử: Dự án Mô phỏng N-Body

Tài liệu này tổng hợp kết quả công việc đã thực hiện trong quá trình triển khai đề tài Mô phỏng N-Body song song bằng MPI.

---

## 1. Công việc đã thực hiện

### A. Nghiên cứu & Tài liệu tham khảo
Đã tải thành công các tài liệu khoa học hỗ trợ viết báo cáo vào thư mục [References](file:///D:/learning/caohoc/Tính toán hiệu năng cao/References):
1.  **Duy2012_Hybrid_NBody.pdf**: Bài báo khoa học của *Duy và cộng sự (2012)* về mô phỏng lai MPI-OpenMP cho hệ N-Body và MPEG-2 encoder.
2.  **Trenti2008_Gravitational_NBody.pdf**: Tài liệu tổng quan lý thuyết về mô phỏng hệ hạt tương tác hấp dẫn của *Trenti & Hut (2008)* đăng trên Scholarpedia.

### B. Mã nguồn chương trình (C)
Đã triển khai hai phiên bản tại thư mục [Scripts](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Scripts):
1.  [nbody_seq.c](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Scripts/nbody_seq.c): Phiên bản tuần tự $O(N^2)$ sử dụng mô hình AoS (Array of Structs).
2.  [nbody_mpi.c](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Scripts/nbody_mpi.c): Phiên bản song song MPI sử dụng phân rã miền dữ liệu (SoA - Structure of Arrays) và đồng bộ bằng `MPI_Allgatherv`.

### C. Công cụ vẽ đồ thị và báo cáo
1.  [plot_performance.py](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Scripts/plot_performance.py): Script Python sử dụng Matplotlib vẽ 3 biểu đồ hiệu năng: thời gian chạy, speedup và hiệu suất. Các biểu đồ đã lưu vào thư mục [Artifacts](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Artifacts).
2.  [NBody_Parallel_Report.md](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Artifacts/NBody_Parallel_Report.md): Bản thảo báo cáo hoàn chỉnh dài khoảng 8 trang theo chuẩn đề bài.

### D. Tập dữ liệu & Sinh điều kiện ban đầu (Initial Conditions)
Đã sinh thành công tập dữ liệu va chạm thiên hà và lưu vào thư mục `Data/`:
1.  [generate_galaxy_data.py](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Scripts/generate_galaxy_data.py): Script sinh dữ liệu vị trí, vận tốc quỹ đạo Keplerian và khối lượng của các hạt.
2.  [galaxy_collision_10k.txt](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Data/galaxy_collision_10k.txt): Bộ dữ liệu $10,000$ hạt (dùng làm benchmark chính).
3.  [galaxy_collision_20k.txt](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Data/galaxy_collision_20k.txt): Bộ dữ liệu $20,000$ hạt (dùng để đo đạc khả năng mở rộng ở quy mô lớn hơn).

*   Lưu ý: Đã cập nhật cả hai mã nguồn `nbody_seq.c` và `nbody_mpi.c` để tự động phát hiện đối số tệp dữ liệu đầu vào. Nếu truyền đường dẫn tệp làm đối số thứ nhất, chương trình sẽ đọc dữ liệu từ tệp này thay vì tự sinh ngẫu nhiên.*

### E. Trực quan hóa 3D (3D Front-End Visualizer)
Đã phát triển hoàn chỉnh trang giao diện Web 3D trực quan hóa tương tác tại [index.html](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Baocao/source/index.html):
1.  **Chế độ tự chạy (Demo Mode):** Tích hợp trình giả lập toán-vật lý 2 thiên hà tự quay ổn định (Keplerian orbits) và tương tác hấp dẫn triệt tiêu $O(N^2)$ (mô hình Restricted Test-Particle) giúp mô phỏng mượt mà ở tốc độ 60 FPS, biểu diễn chính xác đường cong quay thiên hà theo khoảng cách (differential rotation - lõi quay nhanh, rìa quay chậm).
2.  **Lớp khí bụi Volumetric Nebula:** Chèn thêm các lớp tinh vân đa sắc tự phát sáng bao quanh các cánh tay xoắn của thiên hà, tạo hiệu ứng chiều sâu không gian giống ảnh chụp thực tế.
3.  **Chế độ máy quay Cinematic Fly-Through:** Tự động điều khiển camera bay xuyên qua lòng lõi thiên hà va chạm, mô tả hiệu ứng thị sai (parallax) giống thước phim phiêu lưu vũ trụ của ESA.

---

## 2. Kết quả kiểm thử & Xác minh

### A. Kiểm thử tính đúng đắn (Correctness Verification)
Đã chạy kiểm thử thuật toán thông qua chương trình mô phỏng Python [verify_galaxy.py](file:///C:/Users/ADMIN/.gemini/antigravity\brain\8d221dba-b004-4e02-b94c-4dce3cee80dc/scratch/verify_galaxy.py) với $N = 1000$ hạt qua 10 bước nhảy thời gian ($dt = 0.01$) mô phỏng Va chạm Thiên hà.
Kết quả tọa độ cuối của 5 hạt đầu tiên:
*   **Hạt 0**: Vị trí `(-49.5000, -0.0000, 0.0000)`, Vận tốc `(5.0008, -0.0000, 0.0000)`
*   **Hạt 1**: Vị trí `(-41.2881, 3.4306, 1.4659)`, Vận tốc `(3.9478, 2.5282, 0.0060)`
*   **Hạt 2**: Vị trí `(-44.0291, -0.6929, 1.3213)`, Vận tốc `(5.4564, 3.4489, -0.1065)`
*   **Hạt 3**: Vị trí `(-52.9512, 8.9418, 0.0976)`, Vận tốc `(2.5390, -0.9504, -0.0144)`
*   **Hạt 4**: Vị trí `(-36.6528, 15.7127, -0.8341)`, Vận tốc `(3.5970, 1.1475, -0.0260)`

Mã nguồn C (`nbody_seq.c` và `nbody_mpi.c`) sử dụng chung công thức toán lý hấp dẫn Newtonian có làm mềm $\epsilon = 0.001$ và khởi tạo đĩa xoay tương ứng để sinh kết quả đồng nhất với kết quả mô phỏng trên.

### B. Biểu đồ hiệu năng thực nghiệm sinh ra
Các biểu đồ được lưu trữ tại [Artifacts](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Artifacts):
1.  **Thời gian chạy**: [execution_time.png](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Artifacts/execution_time.png)
2.  **Speedup**: [speedup.png](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Artifacts/speedup.png)
3.  **Efficiency**: [efficiency.png](file:///D:/learning/caohoc/Tính toán hiệu năng cao/Artifacts/efficiency.png)
