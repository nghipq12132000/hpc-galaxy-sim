# Hướng dẫn chi tiết thiết lập cụm mô phỏng MPI (FastAPI + C++ MPI) trên AWS EC2
*(Mẫu cấu hình tối ưu giới hạn 8 vCPUs: 1 Master 2 vCPU/8GB RAM & 3 Compute Nodes 2 vCPU/8GB RAM hoặc 16GB RAM)*

Tài liệu này hướng dẫn chi tiết cách tạo các máy ảo AWS EC2 từ giao diện AWS Console, cấu hình mạng, thiết lập SSH không mật khẩu (Passwordless SSH), cài đặt môi trường, biên dịch/chạy mô phỏng song song N-Body trên cụm Cluster với kỹ thuật quá giới hạn vCPU (Oversubscription) và bảng ước tính chi phí.

---

## 📌 Thiết lập cấu hình cụm máy ảo đề xuất (Vừa khít giới hạn 8 vCPUs):
* **1 Master Node (Điều phối, API & Tính toán)**:
  * **Instance Type**: `t3.large` (2 vCPUs, 8 GiB RAM).
  * **Chức năng**: Điều phối hệ thống, chạy FastAPI Backend, đồng thời trực tiếp tham gia mô phỏng tính toán. Việc nâng cấp từ `t3.medium` (4GB RAM) lên `t3.large` (8GB RAM) giúp máy Master có đủ dung lượng bộ nhớ để chạy song song các tiến trình mô phỏng hạt quy mô lớn.
* **3 Compute Nodes (Tính toán MPI)**:
  * **Instance Type**: `t3.large` (2 vCPUs, 8 GiB RAM - Tiết kiệm chi phí) hoặc **`r5.large`** (2 vCPUs, 16 GiB RAM - Dòng tối ưu bộ nhớ Memory Optimized, khuyên dùng khi chạy số lượng hạt rất lớn).
  * **Chức năng**: Chạy song song các tiến trình MPI. Vì giới hạn vCPU của tài khoản chỉ cho phép tối đa 8 vCPUs nên chúng ta chọn loại máy 2 vCPUs. Mỗi máy ảo sẽ chạy vượt mức **4 processes** (Tổng số tiến trình trong cụm là 16 processes).

---

## PHẦN 1: Tạo các máy ảo EC2 trên AWS Console (Click-by-click)

Vì máy Master và các Compute Node sử dụng hai cấu hình khác nhau (hoặc cùng cấu hình nhưng tạo riêng biệt để dễ quản lý), chúng ta sẽ thực hiện khởi tạo qua 2 đợt riêng biệt.

### Bước 1.1: Tạo máy ảo Master (`hpc-master` - 2 vCPU, 8GB RAM)
1. Đăng nhập vào tài khoản [AWS Management Console](https://aws.amazon.com/console/).
2. Trên thanh tìm kiếm ở đỉnh màn hình, gõ `EC2` và click vào dịch vụ **EC2**.
3. Ở góc phải phía trên màn hình, chọn Region **Singapore** hoặc **Tokyo**.
4. Nhấn nút màu cam **Launch instance** ở giữa màn hình.
5. Tại mục **Name and tags**:
   * Nhập tên: **`hpc-master`**.
6. Tại mục **Application and OS Images (AMI)**:
   * Click chọn logo của **Ubuntu**.
   * Dropdown AMI: Chọn **Ubuntu Server 22.04 LTS (HVM), SSD Volume Type** (64-bit x86).
7. Tại mục **Instance type**:
   * Tìm kiếm và chọn **`t3.large`** (2 vCPUs, 8 GiB RAM).
8. Tại mục **Key pair (login)**:
   * Click chọn **Create new key pair** nếu bạn chưa có.
   * **Key pair name**: Gõ `hpc-key`.
   * **Key pair type**: Chọn `RSA`.
   * **Private key file format**: Chọn `.pem`.
   * Nhấn nút **Create key pair**. Một tệp `hpc-key.pem` sẽ tự động được tải về máy của bạn.
9. Tại mục **Network settings**:
   * Click nút **Edit** ở góc phải trên.
   * **Firewall (security groups)**: Chọn **Create security group**.
   * **Security group name**: Nhập `hpc-security-group`.
   * **Description**: Nhập `SG for HPC Galaxy simulation cluster`.
   * Đối với rule SSH có sẵn: Tại **Source type**, chọn **My IP** để chỉ có máy tính của bạn truy cập được.
10. Tại mục **Configure storage**:
    * Mặc định là `8 GiB` gp3. Đổi thành **`15 GiB`** SSD.
11. Nhìn sang cột **Summary** bên phải, kiểm tra xem mục **Number of instances** đang là **`1`** và nhấn nút **Launch instance**.

---

### Bước 1.2: Tạo 3 Compute Nodes (`hpc-node1` đến `hpc-node3` - 2 vCPU, 8GB hoặc 16GB RAM)
1. Nhấn nút **Launch instance** một lần nữa từ màn hình quản lý.
2. Tại mục **Name and tags**:
   * Nhập tên chung: **`hpc-node`**.
3. Tại mục **Application and OS Images (AMI)**:
   * Chọn **Ubuntu** và chọn bản **Ubuntu Server 22.04 LTS (HVM)** như đợt 1.
4. Tại mục **Instance type**:
   * Chọn **`t3.large`** (2 vCPUs, 8 GiB RAM) hoặc chọn **`r5.large`** (2 vCPUs, 16 GiB RAM - Tăng tối đa RAM để chạy khối lượng hạt siêu lớn khi vCPU bị giới hạn).
5. Tại mục **Key pair (login)**:
   * Click chọn key pair **`hpc-key`** đã tạo ở Bước 1.1 (Không cần tạo mới).
6. Tại mục **Network settings**:
   * Click nút **Edit**.
   * **Firewall (security groups)**: Click chọn **Select existing security group** (Chọn SG có sẵn).
   * Tại danh sách bên dưới, click chọn Group **`hpc-security-group`** đã tạo ở Bước 1.1.
7. Tại mục **Configure storage**:
   * Đổi dung lượng ổ cứng thành **`15 GiB`** SSD.
8. Tại cột **Summary** bên phải:
   * Nhập số lượng máy ảo tại ô **Number of instances**: Nhập **`3`**.
   * Nhấn nút màu cam **Launch instance**.
9. Quay lại danh sách quản lý máy ảo (**View all instances**), đổi tên cụ thể từ `hpc-node` thành:
   * `hpc-node1`
   * `hpc-node2`
   * `hpc-node3`

---

## PHẦN 2: Cấu hình Security Group mở cổng giao tiếp cụm (Click-by-click)

Chúng ta cần mở cổng **8000** để Web Dashboard gọi API, cổng **80** phục vụ trang web và **mở toàn bộ traffic nội bộ** giữa các node để MPI giao tiếp dữ liệu.

1. Tại danh sách máy ảo EC2, click chọn máy ảo `hpc-master`.
2. Ở nửa dưới màn hình, click vào tab **Security**.
3. Click chọn ID của Security Group dưới dòng **Security groups** (tên `hpc-security-group`).
4. Nhấn nút **Edit inbound rules** ở góc phải dưới của bảng Inbound Rules.
5. Nhấn nút **Add rule** để thêm quy tắc mới (Rule 2):
   * **Type**: Chọn **HTTP**.
   * **Port**: Mặc định cổng **`80`**.
   * **Source**: Chọn **Anywhere-IPv4** (`0.0.0.0/0`).
6. Nhấn nút **Add rule** tiếp để thêm quy tắc thứ ba (Rule 3):
   * **Type**: Chọn **Custom TCP**.
   * **Port range**: Nhập **`8000`**.
   * **Source**: Chọn **Anywhere-IPv4** (`0.0.0.0/0`).
7. Nhấn nút **Add rule** tiếp để thêm quy tắc thứ tư (Rule 4 - Giao tiếp nội bộ MPI):
   * **Type**: Chọn **All traffic** (Tất cả lưu lượng).
   * **Source**: Chọn **Custom**.
   * Ở ô nhập bên cạnh, gõ **`sg-`** và click chọn ID của chính Security Group hiện tại (Ví dụ: `sg-0123456789abcdef`).
8. Nhấn nút **Save rules** ở góc phải dưới cùng.

---

## PHẦN 3: Thiết lập mạng & SSH nội bộ

Ghi lại **Public IP** của máy `hpc-master` và **Private IP** nội bộ của cả 4 máy.

### Bước 3.1: Đổi quyền tệp khóa Private Key trên máy tính cá nhân
Mở terminal trên máy tính của bạn (PowerShell trên Windows hoặc Terminal trên macOS/Linux) và di chuyển vào thư mục chứa tệp `hpc-key.pem` đã tải về:
* **Đối với Windows (PowerShell)**:
  ```powershell
  icacls.exe .\hpc-key.pem /grant:r "%username%:R"
  icacls.exe .\hpc-key.pem /inheritance:r
  ```
* **Đối với macOS/Linux**:
  ```bash
  chmod 400 hpc-key.pem
  ```

### Bước 3.2: SSH vào máy Master và cấu hình tệp Hosts
SSH vào máy Master bằng lệnh:
```bash
ssh -i hpc-key.pem ubuntu@<IP_Public_Cua_Master>
```
Sau khi đăng nhập thành công vào máy Master, mở tệp hosts nội bộ:
```bash
sudo nano /etc/hosts
```
Di chuyển con trỏ xuống cuối tệp và dán thông tin Private IP của các node (thay thế bằng IP nội bộ thực tế của bạn):
```text
172.31.20.10  master
172.31.20.11  node1
172.31.20.12  node2
172.31.20.13  node3
```
*Nhấn `Ctrl + O`, tiếp tục nhấn `Enter` để lưu, sau đó nhấn `Ctrl + X` để thoát.*

*(Thực hiện tương tự bước chỉnh sửa tệp `/etc/hosts` này trên cả 3 máy compute node bằng cách SSH vào từng máy).*

### Bước 3.3: Tạo khóa SSH nội bộ trên máy `master` và sao chép sang các node
1. Trên máy Master, chạy lệnh sinh cặp khóa RSA nội bộ (Không đặt mật khẩu khi được hỏi):
   ```bash
   ssh-keygen -t rsa -b 2048 -N "" -f ~/.ssh/id_rsa
   ```
2. Thêm khóa này vào danh sách truy cập của chính nó:
   ```bash
   cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
   ```
3. Tạo tệp khóa tạm thời `hpc-key.pem` trên máy Master để làm công cụ SSH chuyển tiếp sang các node:
   ```bash
   nano ~/.ssh/hpc-key.pem
   ```
   *Mở tệp `hpc-key.pem` trên máy cá nhân của bạn, copy toàn bộ nội dung dán vào nano trên Master. Nhấn `Ctrl + O`, `Enter` để lưu, `Ctrl + X` để thoát.*
4. Đặt quyền truy cập cho tệp khóa tạm thời trên Master:
   ```bash
   chmod 400 ~/.ssh/hpc-key.pem
   ```
5. Chạy các lệnh sau để tự động copy khóa công khai từ Master sang các Compute Node:
   ```bash
   ssh-copy-id -i ~/.ssh/id_rsa.pub -o "IdentityFile ~/.ssh/hpc-key.pem" ubuntu@node1
   ssh-copy-id -i ~/.ssh/id_rsa.pub -o "IdentityFile ~/.ssh/hpc-key.pem" ubuntu@node2
   ssh-copy-id -i ~/.ssh/id_rsa.pub -o "IdentityFile ~/.ssh/hpc-key.pem" ubuntu@node3
   ```
   *(Nhập `yes` nếu được hỏi xác thực kết nối lần đầu).*
6. Xóa tệp khóa tạm thời để bảo vệ bảo mật của cụm:
   ```bash
   rm ~/.ssh/hpc-key.pem
   ```

---

## PHẦN 4: Cài đặt phần mềm & Biên dịch mã nguồn

Bạn cần chạy lệnh cài đặt môi trường trên **CẢ 4 MÁYẢO**:

```bash
# Cập nhật danh sách gói phần mềm
sudo apt-get update && sudo apt-get upgrade -y

# Cài đặt bộ công cụ phát triển C++ (GCC/G++) và OpenMPI
sudo apt-get install -y build-essential openmpi-bin openmpi-common libopenmpi-dev git

# Cài đặt Python 3 và Pip (Để chạy API điều phối)
sudo apt-get install -y python3-pip python3-venv python3-dev
```

### Bước 4.1: Đồng bộ hóa mã nguồn sang các Compute Node
1. Trên máy Master, tải mã nguồn từ kho GitHub của bạn:
   ```bash
   cd /home/ubuntu
   git clone https://github.com/nghipq12132000/hpc-galaxy-sim.git
   ```
2. Thực hiện rsync đồng bộ hóa từ máy Master sang các Node tính toán:
   ```bash
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node1:/home/ubuntu/hpc-galaxy-sim/
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node2:/home/ubuntu/hpc-galaxy-sim/
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node3:/home/ubuntu/hpc-galaxy-sim/
   ```

### Bước 4.2: Biên dịch mã nguồn MPI
1. Trên máy Master, biên dịch mã nguồn MPI:
   ```bash
   cd /home/ubuntu/hpc-galaxy-sim
   mpicxx -O3 scripts/nbody_mpi.cpp -o scripts/nbody_mpi
   ```
2. Đồng bộ tệp nhị phân vừa biên dịch sang các Node:
   ```bash
   for node in node1 node2 node3; do
       rsync -avz /home/ubuntu/hpc-galaxy-sim/scripts/nbody_mpi ubuntu@$node:/home/ubuntu/hpc-galaxy-sim/scripts/nbody_mpi
   done
   ```

### Bước 4.3: Chạy ứng dụng FastAPI Backend (Chỉ trên máy Master)
1. Truy cập thư mục backend và kích hoạt môi trường ảo:
   ```bash
   cd /home/ubuntu/hpc-galaxy-sim/backend
   python3 -m venv venv
   source venv/bin/activate
   pip install -r requirements.txt
   ```
2. Chạy Backend API ngầm để phục vụ Dashboard:
   ```bash
   nohup python3 run.py > uvicorn.log 2>&1 &
   ```

---

## PHẦN 5: Ước tính chi phí vận hành cụm 8 vCPUs (AWS Singapore)

Bảng chi phí chi tiết cho **1 giờ** hoạt động cho cả hai phương án RAM:

### Phương án A: Dùng cụm Standard `t3.large` (Master + 3 Nodes đều 8GB RAM)

| Thành phần | Số lượng | Cấu hình máy | Đơn giá / Giờ (USD) | Tổng chi phí / Giờ (USD) | Tổng chi phí / Giờ (VND) |
| :--- | :---: | :--- | :---: | :---: | :---: |
| **Master Node (t3.large)** | 1 | 2 vCPUs, 8 GiB RAM | $0.0832 | **$0.0832** | ~2.080 VND |
| **Compute Nodes (t3.large)** | 3 | 2 vCPUs, 8 GiB RAM | $0.0832 | **$0.2496** | ~6.240 VND |
| **Ổ cứng SSD gp3 (15GB/máy)**| 4 | 60 GB tổng dung lượng | $0.0001 | **$0.0040** | ~100 VND |
| **Data Transfer** | - | Nhận dữ liệu (Free) / Xuất log | - | **$0.0020** | ~50 VND |
| **TỔNG CỘNG** | **4** | **Cụm 8 vCPUs, 32 GiB RAM** | | **$0.3388** | **~8.470 VND** |

### Phương án B: Dùng cụm High-RAM (Master `t3.large` 8GB RAM + 3 Nodes `r5.large` 16GB RAM)

| Thành phần | Số lượng | Cấu hình máy | Đơn giá / Giờ (USD) | Tổng chi phí / Giờ (USD) | Tổng chi phí / Giờ (VND) |
| :--- | :---: | :--- | :---: | :---: | :---: |
| **Master Node (t3.large)** | 1 | 2 vCPUs, 8 GiB RAM | $0.0832 | **$0.0832** | ~2.080 VND |
| **Compute Nodes (r5.large)** | 3 | 2 vCPUs, 16 GiB RAM | $0.1260 | **$0.3780** | ~9.450 VND |
| **Ổ cứng SSD gp3 (15GB/máy)**| 4 | 60 GB tổng dung lượng | $0.0001 | **$0.0040** | ~100 VND |
| **Data Transfer** | - | Nhận dữ liệu (Free) / Xuất log | - | **$0.0020** | ~50 VND |
| **TỔNG CỘNG** | **4** | **Cụm 8 vCPUs, 56 GiB RAM** | | **$0.4672** | **~11.680 VND** |

### 💾 Chi phí duy trì Ổ cứng SSD gp3 khi TẮT máy (Stop Instance) trong 7 ngày:
* **Tổng dung lượng**: 60 GB SSD cho cụm 4 máy.
* Giá lưu trữ ổ đĩa gp3 SSD tại Singapore là **$0.08 / GB / tháng**.
* **Chi phí duy trì 1 tuần (7 ngày)**: $60\text{ GB} \times 0.08 \times (7 / 30) \approx \mathbf{\$1.12\text{ USD}}$ (khoảng **~28.000 VND**).

---

## PHẦN 6: Cách chọn cấu hình tối ưu hiệu năng trên Dashboard

Vì máy Master bây giờ cũng trực tiếp tham gia tính toán mô phỏng và chạy song song các tiến trình MPI, bạn hãy cấu hình trên Dashboard như sau để chia đều 16 processes lên 4 máy ảo:

1. **Thiết lập thanh trượt trên Dashboard**:
   * **Compute Nodes**: Kéo chọn **`4`** nodes.
   * **Processes (p)**: Kéo chọn **`16`** processes.
2. **Cấu hình địa chỉ IP (Cluster Network)**:
   * **Master**: Điền Private IP của máy Master.
   * **Node 1**: Điền **Private IP của máy Master** (để Master hoạt động như một node tính toán thực thụ).
   * **Node 2**: Điền Private IP của máy `node1`.
   * **Node 3**: Điền Private IP của máy `node2`.
   * **Node 4**: Điền Private IP của máy `node3`.
3. Sơ đồ **MPI Ranks Mapping** lúc này sẽ tự động phân phối:
   * **Master (Node 1)**: Chạy 4 MPI processes (`Rank 0` tới `Rank 3`).
   * **Compute Node 1 (Node 2)**: Chạy 4 MPI processes (`Rank 4` tới `Rank 7`).
   * **Compute Node 2 (Node 3)**: Chạy 4 MPI processes (`Rank 8` tới `Rank 11`).
   * **Compute Node 3 (Node 4)**: Chạy 4 MPI processes (`Rank 12` tới `Rank 15`).
   * *Tổng cộng*: 16 processes chạy song song trên cụm 8 vCPUs vật lý.

### 💡 Tại sao cấu hình này hoạt động được? (Kỹ thuật Oversubscription)
OpenMPI hỗ trợ tính năng chạy vượt mức số lượng nhân vật lý (Oversubscribing). Trong mã nguồn điều phối backend của bạn tại [simulation.py](file:///g:/My%20Drive/Classroom/T%C3%ADnh%20to%C3%A1n%20hi%E1%BB%87u%20n%C4%83ng%20cao/hpc-galaxy-sim/backend/app/services/simulation.py#L53), cờ `--oversubscribe` đã được thêm vào lệnh khởi chạy `mpirun` (`mpirun --oversubscribe ...`). Điều này cho phép mỗi máy ảo 2 vCPUs vẫn có thể chạy mượt mà 4 tiến trình MPI đồng thời mà không bị ngắt kết nối hay báo lỗi giới hạn luồng của CPU.

---

## PHẦN 7: Cấu hình Web Server Nginx trên máy Master

Để xem được giao diện mô phỏng 3D từ bất kỳ trình duyệt nào mà không cần chạy local file `index.html`, chúng ta sẽ cấu hình Nginx làm Web Server và Reverse Proxy:

### Bước 7.1: Cài đặt Nginx trên máy Master
```bash
sudo apt-get update
sudo apt-get install -y nginx
```

### Bước 7.2: Copy mã nguồn Front-end vào thư mục Web của Nginx
```bash
sudo cp /home/ubuntu/hpc-galaxy-sim/frontend/index.html /var/www/html/index.html
sudo cp /home/ubuntu/hpc-galaxy-sim/frontend/.env /var/www/html/.env
```

### Bước 7.3: Cấu hình Nginx làm Reverse Proxy
Mở tệp cấu hình mặc định của Nginx:
```bash
sudo nano /etc/nginx/sites-available/default
```
Thay thế toàn bộ nội dung của tệp bằng cấu hình dưới đây:
```nginx
server {
    listen 80 default_server;
    listen [::]:80 default_server;

    root /var/www/html;
    index index.html;

    server_name _;

    # Phục vụ giao diện tĩnh Web Dashboard
    location / {
        try_files $uri $uri/ =404;
    }

    # Reverse Proxy chuyển tiếp yêu cầu API sang FastAPI Backend
    location /api {
        proxy_pass http://127.0.0.1:8000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }
}
```

### Bước 7.4: Kiểm tra và khởi động lại Nginx
1. Kiểm tra cú pháp:
   ```bash
   sudo nginx -t
   ```
2. Khởi động lại dịch vụ:
   ```bash
   sudo systemctl restart nginx
   ```

Giờ đây bạn chỉ cần gõ địa chỉ IP công cộng của Master trên trình duyệt:  
`http://<IP_Public_Cua_Master>`  
Giao diện 3D sẽ tự động kết nối hoàn hảo với API backend ở cổng 8000.
