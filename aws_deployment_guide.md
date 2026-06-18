# Hướng dẫn chi tiết thiết lập cụm mô phỏng MPI (FastAPI + C++ MPI) trên AWS EC2
*(Mẫu cấu hình: Master 2 vCPU/4GB RAM & 4 Compute Nodes 4 vCPU/8GB RAM)*

Tài liệu này hướng dẫn chi tiết cách tạo các máy ảo AWS EC2 từ giao diện AWS Console, cấu hình mạng, thiết lập SSH không mật khẩu (Passwordless SSH), cài đặt môi trường, biên dịch/chạy mô phỏng song song N-Body trên cụm Cluster và bảng ước tính chi phí.

---

## 📌 Thiết lập cấu hình cụm máy ảo đề xuất:
* **1 Master Node (Điều phối & API)**:
  * **Instance Type**: `t3.medium` (2 vCPUs, 4 GiB RAM).
  * **Chức năng**: Điều phối hệ thống, chạy FastAPI Backend, không trực tiếp tham gia mô phỏng nặng.
* **4 Compute Nodes (Tính toán MPI)**:
  * **Instance Type**: `c5.xlarge` (4 vCPUs, 8 GiB RAM - Dòng tối ưu tính toán) hoặc `t3.xlarge` (4 vCPUs, 16 GiB RAM).
  * **Chức năng**: Chạy song song các tiến trình MPI. Mỗi máy ảo có 4 vCPUs nên sẽ chạy tối ưu **4 processes** (Tổng số tiến trình trong cụm $p = 16$ processes).

---

## PHẦN 1: Tạo các máy ảo EC2 trên AWS Console (Click-by-click)

Vì máy Master và các Compute Node sử dụng hai cấu hình khác nhau, chúng ta sẽ thực hiện khởi tạo qua 2 đợt riêng biệt.

### Bước 1.1: Tạo máy ảo Master (`hpc-master` - 2 vCPU, 4GB RAM)
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
   * Tìm kiếm và chọn **`t3.medium`** (2 vCPUs, 4 GiB RAM).
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

### Bước 1.2: Tạo 4 Compute Nodes (`hpc-node1` đến `hpc-node4` - 4 vCPU)
Ngay sau khi máy Master được tạo xong, tiếp tục tạo đợt 2 cho các Compute Node tính toán:
1. Nhấn nút **Launch instance** một lần nữa từ màn hình quản lý.
2. Tại mục **Name and tags**:
   * Nhập tên chung: **`hpc-node`**.
3. Tại mục **Application and OS Images (AMI)**:
   * Chọn **Ubuntu** và chọn bản **Ubuntu Server 22.04 LTS (HVM)** như đợt 1.
4. Tại mục **Instance type**:
   * Tìm kiếm và chọn dòng máy **`c5.xlarge`** (4 vCPUs, 8 GiB RAM - Tối ưu nhất cho các phép tính số học mô phỏng) hoặc chọn **`t3.xlarge`** (4 vCPUs, 16 GiB RAM).
5. Tại mục **Key pair (login)**:
   * Click chọn key pair **`hpc-key`** đã tạo ở Bước 1.1 (Không cần tạo mới key pair).
6. Tại mục **Network settings**:
   * Click nút **Edit**.
   * **Firewall (security groups)**: Click chọn **Select existing security group** (Chọn SG có sẵn).
   * Tại danh sách bên dưới, click chọn Group **`hpc-security-group`** mà chúng ta đã tạo cho máy Master ở bước trước. *(Điều này đảm bảo các Compute Node dùng chung luật bảo mật và dễ dàng giao tiếp mạng nội bộ với Master).*
7. Tại mục **Configure storage**:
   * Đổi dung lượng ổ cứng thành **`15 GiB`** SSD.
8. Tại cột **Summary** bên phải:
   * Nhập số lượng máy ảo tại ô **Number of instances**: Nhập **`4`**.
   * Nhấn nút màu cam **Launch instance**.
9. Quay lại danh sách quản lý máy ảo (**View all instances**), bạn sẽ thấy có 4 máy ảo mới được tạo. Hãy click vào hình bút chì tại cột *Name* của từng máy ảo mới này để đổi tên cụ thể từ `hpc-node` thành:
   * `hpc-node1`
   * `hpc-node2`
   * `hpc-node3`
   * `hpc-node4`

---

## PHẦN 2: Cấu hình Security Group mở cổng giao tiếp cụm (Click-by-click)

Chúng ta cần mở cổng **8000** để Web Dashboard gọi API và **mở toàn bộ traffic nội bộ** giữa các node để MPI giao tiếp dữ liệu.

1. Tại danh sách máy ảo EC2, click chọn máy ảo `hpc-master`.
2. Ở nửa dưới màn hình, click vào tab **Security**.
3. Click chọn ID của Security Group dưới dòng **Security groups** (tên `hpc-security-group`).
4. Nhấn nút **Edit inbound rules** ở góc phải dưới của bảng Inbound Rules.
5. Nhấn nút **Add rule** để thêm quy tắc mới (Rule 2):
   * **Type**: Chọn **Custom TCP**.
   * **Port range**: Nhập **`8000`**.
   * **Source**: Chọn **Anywhere-IPv4** (`0.0.0.0/0`).
6. Nhấn nút **Add rule** tiếp để thêm quy tắc thứ ba (Rule 3 - Quy tắc quan trọng nhất cho MPI):
   * **Type**: Chọn **All traffic** (Tất cả lưu lượng).
   * **Source**: Chọn **Custom**.
   * Ở ô nhập bên cạnh, gõ **`sg-`** hệ thống sẽ tự động hiển thị gợi ý ID của chính Security Group hiện tại. Click chọn nó (Ví dụ: `sg-0123456789abcdef`).
   * *Ý nghĩa*: Luật này cho phép tất cả các máy ảo sử dụng chung Security Group này truyền nhận dữ liệu qua mọi cổng mạng với nhau (giao thức MPI).
7. Nhấn nút **Save rules** ở góc phải dưới cùng.

---

## PHẦN 3: Thiết lập mạng & SSH nội bộ

Ghi lại **Public IP** của máy `hpc-master` và **Private IP** nội bộ của cả 5 máy.

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
172.31.20.14  node4
```
*Nhấn `Ctrl + O`, tiếp tục nhấn `Enter` để lưu, sau đó nhấn `Ctrl + X` để thoát.*

*(Thực hiện tương tự bước chỉnh sửa tệp `/etc/hosts` này trên cả 4 máy compute node bằng cách SSH vào từng máy).*

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
   ssh-copy-id -i ~/.ssh/id_rsa.pub -o "IdentityFile ~/.ssh/hpc-key.pem" ubuntu@node4
   ```
   *(Nhập `yes` nếu được hỏi xác thực kết nối lần đầu).*
6. Xóa tệp khóa tạm thời để bảo vệ bảo mật của cụm:
   ```bash
   rm ~/.ssh/hpc-key.pem
   ```

---

## PHẦN 4: Cài đặt phần mềm & Biên dịch mã nguồn

Bạn cần chạy lệnh cài đặt môi trường trên **CẢ 5 MÁYẢO**:

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
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node4:/home/ubuntu/hpc-galaxy-sim/
   ```

### Bước 4.2: Biên dịch mã nguồn MPI
1. Trên máy Master, biên dịch mã nguồn MPI:
   ```bash
   cd /home/ubuntu/hpc-galaxy-sim
   mpicxx -O3 scripts/nbody_mpi.cpp -o scripts/nbody_mpi
   ```
2. Đồng bộ tệp nhị phân vừa biên dịch sang các Node:
   ```bash
   for node in node1 node2 node3 node4; do
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

## PHẦN 5: Ước tính chi phí vận hành cụm Cluster trong 1 giờ (AWS Singapore)

Cấu hình cụm gồm: **1 máy `master` (`t3.medium` - 2 vCPU)** và **4 máy `node` (`c5.xlarge` - 4 vCPU / 8GB RAM mỗi máy)**.

Bảng chi phí chi tiết cho **1 giờ** hoạt động:

| Thành phần | Số lượng | Cấu hình máy | Đơn giá / Giờ (USD) | Tổng chi phí / Giờ (USD) | Tổng chi phí / Giờ (VND) |
| :--- | :---: | :--- | :---: | :---: | :---: |
| **Master Node (t3.medium)** | 1 | 2 vCPUs, 4 GiB RAM | $0.0416 | **$0.0416** | ~1.040 VND |
| **Compute Nodes (c5.xlarge)** | 4 | 4 vCPUs, 8 GiB RAM | $0.1700 | **$0.6800** | ~17.000 VND |
| **Ổ cứng SSD gp3 (15GB/máy)** | 5 | 75 GB tổng dung lượng | $0.0001 | **$0.0050** | ~120 VND |
| **Data Transfer** | - | Nhận dữ liệu (Free) / Xuất log | - | **$0.0020** | ~50 VND |
| **TỔNG CỘNG** | **5** | **Cụm 18 vCPUs, 36 GiB RAM** | | **$0.7286** | **~18.210 VND** |

### Cách chọn cấu hình tối ưu hiệu năng trên Dashboard:
* Khi mở Web Dashboard cá nhân của bạn, hãy nhập IP của Master và các compute nodes.
* Tại cột trái của dashboard, cấu hình:
  * **Compute Nodes**: Kéo chọn **`4`** nodes.
  * **Processes (p)**: Kéo chọn **`16`** processes.
* Sơ đồ **MPI Ranks Mapping** sẽ tự động tính toán phân bố đều: mỗi máy ảo Compute Node chạy đúng **4 MPI processes** tương ứng với 4 vCPUs của dòng máy ảo `c5.xlarge` đã tạo, giúp tận dụng tối đa 100% công suất tính toán của cụm AWS Cluster!

### 💾 Chi phí duy trì Ổ cứng SSD gp3 khi TẮT máy (Stop Instance) từ 18/6 đến 25/6 (7 ngày):

Khi bạn **Tắt máy ảo (Stop)** để tạm nghỉ, CPU và RAM của máy ảo sẽ được giải phóng hoàn toàn và không tính phí. Tuy nhiên, ổ cứng lưu trữ SSD (EBS gp3 Volume) vẫn được AWS giữ lại để bảo vệ hệ điều hành, môi trường OpenMPI và code của bạn. Phần dung lượng này **vẫn sẽ bị tính phí**.

Giá lưu trữ ổ đĩa gp3 SSD tại Singapore là **$0.08 / GB / tháng** (30 ngày). 
Dưới đây là chi phí dự tính cho 1 tuần (18/6 - 25/6) duy trì cụm khi tắt máy:

* **Đối với Cụm 5 máy ảo (1 Master + 4 Compute Nodes - Tổng 75 GB SSD)**:
  * Chi phí 1 tháng: $75\text{ GB} \times 0.08 = \$6.00\text{ USD}$ (~150.000 VND).
  * Chi phí 1 ngày: $\$6.00 / 30 \text{ ngày} = \$0.20\text{ USD}$ (~5.000 VND).
  * **Chi phí duy trì 1 tuần (18/6 - 25/6)**: $\$0.20 \times 7\text{ ngày} = \mathbf{\$1.40\text{ USD}}$ (khoảng **~35.000 VND**).

* **Đối với Cụm 4 máy ảo (1 Master + 3 Compute Nodes - Tổng 60 GB SSD)**:
  * Chi phí 1 tháng: $60\text{ GB} \times 0.08 = \$4.80\text{ USD}$ (~120.000 VND).
  * Chi phí 1 ngày: $\$4.80 / 30 \text{ ngày} = \$0.16\text{ USD}$ (~4.000 VND).
  * **Chi phí duy trì 1 tuần (18/6 - 25/6)**: $\$0.16 \times 7\text{ ngày} = \mathbf{\$1.12\text{ USD}}$ (khoảng **~28.000 VND**).

*Mẹo tiết kiệm*: Nếu bạn muốn ngắt hoàn toàn mọi chi phí phát sinh khi dừng nghiên cứu lâu ngày, bạn có thể tạo một bản sao lưu ảnh đĩa (Create AMI) của các máy ảo rồi Terminate (Xóa hẳn) máy ảo đó đi. Khi cần làm việc lại, bạn chỉ cần khởi chạy cụm máy mới từ AMI đã sao lưu mà không phải cài đặt lại từ đầu!

---

## 🛠 Xử lý lỗi: "vCPU capacity limit exceeded (Limit: 8 vCPUs)"
Nếu bạn sử dụng tài khoản AWS mới hoặc tài khoản Free Tier, AWS mặc định chỉ cấp hạn mức sử dụng tối đa **8 vCPUs** cho toàn bộ các máy ảo của bạn. 
* Máy Master (`t3.medium`) đã sử dụng **2 vCPUs**.
* Nếu bạn khởi tạo 4 Compute Nodes loại `c5.xlarge` (mỗi node 4 vCPUs $\times$ 4 = 16 vCPUs), tổng số vCPU yêu cầu là 18 vCPUs, vượt quá giới hạn 8 vCPUs nên AWS báo lỗi `Instance launch failed`.

### 💡 Giải pháp 1: Giảm số Node xuống 3 máy và hạ cấu hình (Khuyên dùng - Không cần nâng hạn mức)
Bạn có thể cấu hình cụm Cluster của mình vừa khít với **8 vCPUs** giới hạn bằng cách:
1. **Master Node**: Vẫn giữ nguyên **`t3.medium`** (2 vCPUs, 4 GiB RAM).
2. **Compute Nodes**: Giảm xuống **3 Compute Nodes** (node1, node2, node3) và sử dụng loại máy ảo **`t3.medium`** hoặc **`c5.large`** (đều có **2 vCPUs**, 4 GiB RAM).
   * **Tổng số vCPUs**: $2 \text{ (Master)} + 3 \text{ Nodes} \times 2 = \mathbf{8 \text{ vCPUs}}$ (Đạt ngưỡng tối đa cho phép).
   * **Cấu hình trên Dashboard**: Chọn **`3` Compute Nodes** và **`6` Processes** (Mỗi Node chạy 2 processes tương ứng 2 vCPU).

Bảng chi phí 1 giờ cho giải pháp này:

| Máy ảo | Số lượng | Cấu hình | Đơn giá / Giờ | Tổng cộng / Giờ |
| :--- | :---: | :--- | :---: | :---: |
| **Master Node (t3.medium)** | 1 | 2 vCPUs, 4 GiB RAM | $0.0416 | **$0.0416** (~1.040đ) |
| **Compute Nodes (c5.large)** | 3 | 2 vCPUs, 4 GiB RAM | $0.0850 | **$0.2550** (~6.370đ) |
| **Ổ cứng SSD gp3** | 4 | 60 GB tổng | $0.0001 | **$0.0040** (~100đ) |
| **TỔNG CỘNG** | **4** | **Cụm 8 vCPUs, 16 GiB RAM** | | **$0.3006** (~7.510đ) |

### 💡 Giải pháp 2: Giữ nguyên 4 Compute Nodes nhưng dùng máy 1 vCPU
Nếu bạn bắt buộc phải demo cụm 4 Compute Nodes (tổng cộng 5 máy ảo bao gồm cả Master):
1. **Master Node**: Chọn loại **`t2.small`** (1 vCPU, 2 GiB RAM).
2. **Compute Nodes**: Chọn 4 máy loại **`t2.micro`** (1 vCPU, 1 GiB RAM).
   * **Tổng số vCPUs**: $1 \text{ (Master)} + 4 \times 1 \text{ (Nodes)} = \mathbf{5 \text{ vCPUs}}$ (Thoải mái nằm trong hạn mức 8 vCPUs).
   * **Cấu hình trên Dashboard**: Chọn **`4` Compute Nodes** và **`4` Processes** (Mỗi Node chạy 1 process tương ứng 1 vCPU).

### 💡 Giải pháp 3: Gửi yêu cầu nâng hạn mức vCPU của AWS (Miễn phí)
1. Click vào liên kết báo lỗi trong AWS Console hoặc truy cập: [http://aws.amazon.com/contact-us/ec2-request](http://aws.amazon.com/contact-us/ec2-request)
2. Chọn loại yêu cầu nâng hạn mức cho **Standard (A, C, D, H, I, M, R, T, Z) instances**.
3. Đề xuất nâng lên hạn mức **`32` vCPUs** hoặc **`64` vCPUs**. AWS thường tự động duyệt yêu cầu này của bạn trong vòng **1 đến 2 giờ** hoàn toàn miễn phí. Sau đó bạn có thể tạo cụm cấu hình cao `c5.xlarge` bình thường.

---

## PHẦN 6: Cấu hình Web Server Nginx trên máy Master

Để biến máy Master thành máy chủ lưu trữ Web thực tế, giúp bạn và thầy cô chỉ cần nhập địa chỉ IP của máy Master (ví dụ: `http://<Master_Public_IP>`) là xem được giao diện mô phỏng 3D ngay lập tức (không cần chạy local file `index.html`), chúng ta sẽ cấu hình Nginx làm Web Server và Reverse Proxy:

### Bước 6.1: Cài đặt Nginx trên máy Master
SSH vào máy Master và chạy các lệnh:
```bash
sudo apt-get update
sudo apt-get install -y nginx
```

### Bước 6.2: Copy mã nguồn Front-end vào thư mục Web của Nginx
Copy tệp giao diện `index.html` và tệp cấu hình `.env` vào thư mục gốc chứa trang web mặc định của Nginx để trình duyệt có thể truy cập được:
```bash
sudo cp /home/ubuntu/hpc-galaxy-sim/frontend/index.html /var/www/html/index.html
sudo cp /home/ubuntu/hpc-galaxy-sim/frontend/.env /var/www/html/.env
```

### Bước 6.3: Cấu hình Nginx làm Reverse Proxy
Chúng ta sẽ cấu hình Nginx tự động chuyển tiếp các cổng:
* Truy cập `http://<IP_Master>` $\rightarrow$ Hiển thị giao diện Web Dashboard (Cổng 80).
* Truy cập `http://<IP_Master>/api/...` $\rightarrow$ Nginx tự động chuyển hướng nội bộ sang FastAPI Backend đang chạy ở cổng 8000 (`http://127.0.0.1:8000/api/...`).
* *Ưu điểm*: Bỏ qua lỗi CORS, không cần mở cổng 8000 trên AWS Security Group nữa (chỉ cần mở cổng 80 chuẩn Web).

Mở tệp cấu hình mặc định của Nginx:
```bash
sudo nano /etc/nginx/sites-available/default
```
Tìm đến khối cấu hình `server { ... }` và thay thế toàn bộ nội dung của tệp bằng cấu hình dưới đây:
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
*Nhấn `Ctrl + O`, `Enter` để lưu, `Ctrl + X` để thoát.*

### Bước 6.4: Kiểm tra và khởi động lại Nginx
1. Kiểm tra cú pháp cấu hình xem có bị lỗi chính tả không:
   ```bash
   sudo nginx -t
   ```
   *(Nếu thấy hiển thị `syntax is ok` và `test is successful` là cấu hình chuẩn).*
2. Khởi động lại dịch vụ Nginx để áp dụng cấu hình mới:
   ```bash
   sudo systemctl restart nginx
   ```

### 🎈 Hưởng thành quả:
* **Mở cổng 80 trên AWS**: Đảm bảo Security Group của máy Master đã được mở cổng **HTTP (cổng 80)** cho Anywhere (`0.0.0.0/0`).
* **Truy cập**: Giờ đây bạn chỉ cần gõ địa chỉ IP công cộng của Master trên trình duyệt máy tính của bạn:  
  `http://<IP_Public_Cua_Master>`  
  Giao diện 3D Three.js sẽ hiện ra ngay lập tức và tự động kết nối hoàn hảo với API ở backend!


