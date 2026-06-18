# Hướng dẫn thiết lập cụm mô phỏng MPI (FastAPI + C++ MPI) trên AWS EC2

Tài liệu này hướng dẫn chi tiết cách tạo các máy ảo AWS EC2, cấu hình mạng, thiết lập SSH không mật khẩu (Passwordless SSH), cài đặt môi trường và biên dịch/chạy mô phỏng song song N-Body trên cụm Cluster.

---

## 📌 Kiến trúc hệ thống mẫu
* **1 Master Node (Điều phối)**: Chạy FastAPI Backend API, nhận yêu cầu từ Web Dashboard.
* **4 Compute Nodes (Tính toán)**: `node1`, `node2`, `node3`, `node4` chịu trách nhiệm chạy các tiến trình MPI.
* **Hệ điều hành khuyến nghị**: Ubuntu Server 22.04 LTS (Hỗ trợ tốt nhất cho OpenMPI và Python).

---

## Bước 1: Khởi tạo các AWS EC2 Instances

### 1. Cấu hình Instance
Truy cập AWS Console và tiến hành Launch Instances với các thông số:
* **Name**: `hpc-master`, `hpc-node1`, `hpc-node2`, `hpc-node3`, `hpc-node4`.
* **OS Image**: Ubuntu Server 22.04 LTS.
* **Instance Type**: 
  * `t3.medium` (2 vCPUs, 4GB RAM) hoặc cao hơn (ví dụ: dòng `c5.large` chuyên tính toán) để đảm bảo tốc độ mô phỏng.
* **Key pair**: Tạo một Key pair mới (ví dụ: `hpc-cluster.pem`) và tải về máy.

### 2. Cấu hình Security Group (Cực kỳ quan trọng)
Tất cả các máy ảo trong cụm cần nằm chung một Security Group với cấu hình luật (Inbound Rules) như sau:
1. **Luật SSH**: Cho phép cổng `22` từ địa chỉ IP của bạn (My IP) để truy cập dòng lệnh.
2. **Luật Backend API**: Cho phép cổng `8000` (hoặc cổng chạy FastAPI) từ IP của bạn hoặc Anywhere (`0.0.0.0/0`) để Web Dashboard ở máy cá nhân gọi API.
3. **Luật Nội bộ Cluster (Cho phép giao tiếp MPI)**:
   * Thêm luật: **All traffic** (Tất cả lưu lượng).
   * Source (Nguồn): **Custom** và nhập **ID của chính Security Group này** (ví dụ: `sg-0123456789abcdef`).
   * *Giải thích*: MPI sử dụng các cổng ngẫu nhiên rất cao để giao tiếp dữ liệu giữa các tiến trình. Việc mở toàn bộ traffic nội bộ giữa các EC2 có chung Security Group là bắt buộc.

---

## Bước 2: Thiết lập IP tĩnh nội bộ và Cấu hình Hosts

Khi khởi chạy, AWS sẽ cấp cho mỗi EC2 một **Private IP** (IP nội bộ) dạng `172.31.X.X`. Hãy ghi lại danh sách Private IP này của tất cả các node.

Truy cập vào từng node qua SSH và cập nhật tệp `/etc/hosts` để các node nhận diện tên của nhau thay vì nhớ IP:
```bash
sudo nano /etc/hosts
```
Thêm vào cuối tệp danh sách IP nội bộ của cụm (thay thế bằng IP thực tế của bạn):
```text
172.31.20.10  master
172.31.20.11  node1
172.31.20.12  node2
172.31.20.13  node3
172.31.20.14  node4
```

---

## Bước 3: Cấu hình SSH không mật khẩu (Passwordless SSH)

Tiến trình MPI khởi chạy bằng lệnh `mpirun` trên máy `master` sẽ cần SSH tự động sang các máy `node1`...`node4` để chạy tiến trình con mà không được hỏi mật khẩu.

### 1. Tạo SSH Key trên máy `master`
SSH vào máy `master` và chạy lệnh sinh key:
```bash
ssh-keygen -t rsa -b 2048 -N "" -f ~/.ssh/id_rsa
```

### 2. Copy Key từ `master` sang tất cả các Compute Nodes
Copy khóa công khai (`id_rsa.pub`) từ `master` sang file `authorized_keys` của chính nó và tất cả các node còn lại:
```bash
# SSH vào chính master không mật khẩu
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys

# Copy sang các compute nodes (Sử dụng tệp hpc-cluster.pem từ máy cá nhân của bạn để trung chuyển hoặc copy thủ công)
# Cách chuyên nghiệp: Trên máy master, tạo tạm tệp private key của AWS để copy:
nano ~/.ssh/hpc-cluster.pem   # Dán nội dung tệp hpc-cluster.pem vào đây
chmod 400 ~/.ssh/hpc-cluster.pem

# Copy sang từng node
ssh-copy-id -i ~/.ssh/id_rsa.pub -o "IdentityFile ~/.ssh/hpc-cluster.pem" ubuntu@node1
ssh-copy-id -i ~/.ssh/id_rsa.pub -o "IdentityFile ~/.ssh/hpc-cluster.pem" ubuntu@node2
ssh-copy-id -i ~/.ssh/id_rsa.pub -o "IdentityFile ~/.ssh/hpc-cluster.pem" ubuntu@node3
ssh-copy-id -i ~/.ssh/id_rsa.pub -o "IdentityFile ~/.ssh/hpc-cluster.pem" ubuntu@node4

# Xóa tệp private key tạm trên master để bảo mật
rm ~/.ssh/hpc-cluster.pem
```

### 3. Xác minh kết nối SSH không mật khẩu
Từ máy `master`, kiểm tra xem có thể SSH tới các node mà không cần mật khẩu hay không:
```bash
ssh node1 "hostname"
ssh node2 "hostname"
ssh node3 "hostname"
ssh node4 "hostname"
```
*(Nếu mỗi lệnh in ra đúng hostname của node tương ứng mà không yêu cầu nhập mật khẩu hay xác nhận YES/NO là thành công).*

---

## Bước 4: Cài đặt Môi trường (Cài đặt trên TẤT CẢ các Node)

Bạn cần chạy các lệnh cài đặt dưới đây trên **cả 5 máy ảo** (`master`, `node1`...`node4`):

```bash
# Cập nhật hệ thống
sudo apt-get update && sudo apt-get upgrade -y

# Cài đặt trình biên dịch C++ và OpenMPI
sudo apt-get install -y build-essential openmpi-bin openmpi-common libopenmpi-dev git

# Cài đặt Python và Pip (Để chạy backend FastAPI)
sudo apt-get install -y python3-pip python3-venv python3-dev
```

Kiểm tra phiên bản MPI đã được cài đặt thành công:
```bash
mpicxx --version
mpirun --version
```

---

## Bước 5: Cấu hình Thư mục dự án đồng nhất

Để MPI chạy thành công đa node, **mã nguồn, tệp dữ liệu đầu vào và tệp thực thi biên dịch phải nằm ở cùng một đường dẫn tuyệt đối trên tất cả các node** (ví dụ: `/home/ubuntu/hpc-galaxy-sim`).

### Cách 1: Sử dụng Hệ thống file chia sẻ NFS (Khuyến nghị cho Cluster)
Giúp chia sẻ thư mục `/home/ubuntu/hpc-galaxy-sim` từ `master` sang các `node` qua mạng, không cần copy thủ công.
*(Tham khảo cài đặt nhanh NFS-Kernel-Server trên Master và NFS-Common trên các Node).*

### Cách 2: Sao chép thủ công (Đơn giản nhất để thử nghiệm)
1. Trên máy `master`, tải mã nguồn từ Git về thư mục home:
   ```bash
   cd /home/ubuntu
   git clone https://github.com/nghipq12132000/hpc-galaxy-sim.git
   ```
2. Thực hiện đồng bộ thư mục dự án từ `master` sang các node bằng lệnh `rsync`:
   ```bash
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node1:/home/ubuntu/hpc-galaxy-sim/
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node2:/home/ubuntu/hpc-galaxy-sim/
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node3:/home/ubuntu/hpc-galaxy-sim/
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node4:/home/ubuntu/hpc-galaxy-sim/
   ```

---

## Bước 6: Biên dịch mã nguồn C++ MPI

Trên máy `master`, truy cập thư mục dự án và tiến hành biên dịch mã nguồn MPI:
```bash
cd /home/ubuntu/hpc-galaxy-sim
mpicxx -O3 scripts/nbody_mpi.cpp -o scripts/nbody_mpi
```

Sau khi biên dịch xong trên `master`, hãy đồng bộ tệp thực thi `nbody_mpi` sang tất cả các compute nodes để đảm bảo cấu trúc giống hệt nhau:
```bash
# Đồng bộ tệp thực thi mới sang các node tính toán
for node in node1 node2 node3 node4; do
    rsync -avz /home/ubuntu/hpc-galaxy-sim/scripts/nbody_mpi ubuntu@$node:/home/ubuntu/hpc-galaxy-sim/scripts/nbody_mpi
done
```

---

## Bước 7: Cài đặt và Khởi chạy Backend FastAPI (Chỉ trên máy `master`)

1. Truy cập thư mục backend:
   ```bash
   cd /home/ubuntu/hpc-galaxy-sim/backend
   ```
2. Tạo và kích hoạt môi trường ảo Python:
   ```bash
   python3 -m venv venv
   source venv/bin/activate
   ```
3. Cài đặt các thư viện phụ thuộc:
   ```bash
   pip install -r requirements.txt
   ```
4. Khởi chạy Uvicorn server chạy ngầm ở cổng `8000`:
   ```bash
   # Chạy ngầm server bằng nohup để không bị tắt khi ngắt SSH
   nohup python3 run.py > uvicorn.log 2>&1 &
   ```
   *Mẹo*: Bạn có thể kiểm tra xem backend chạy thành công hay chưa bằng cách xem file log: `tail -f uvicorn.log`.

---

## Bước 8: Kết nối Web Dashboard với AWS Cluster

1. Mở tệp `frontend/index.html` bằng trình duyệt trên máy tính cá nhân của bạn.
2. Tại bảng điều khiển bên trái, cấu hình các thông số mạng:
   * **Master IP**: Nhập **Public IP** của máy `master` (IP ngoại网 để kết nối API).
   * **Node 1 IP**: Nhập Private IP của `node1` (ví dụ: `172.31.20.11`).
   * **Node 2 IP**: Nhập Private IP của `node2`.
   * **Node 3 IP**: Nhập Private IP của `node3`.
   * **Node 4 IP**: Nhập Private IP của `node4`.
3. Nhấp chọn số lượng Tiến trình (`Processes`) và số lượng `Compute Nodes` muốn phân bổ.
4. Giao diện **MPI Ranks Mapping** sẽ hiển thị sơ đồ phân chia luồng tính toán từ `R0` trở đi bắt đầu tại Node 1 (Master chỉ làm nhiệm vụ coordinator điều phối lệnh).
5. Nhấp nút **Run MPI** để kích hoạt cuộc mô phỏng. Logs stdout tính toán song song từ cụm Cluster AWS sẽ được truyền về hiển thị thời gian thực trên màn hình Terminal của Web Dashboard!
