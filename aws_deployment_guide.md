# Hướng dẫn chi tiết thiết lập cụm mô phỏng MPI (FastAPI + C++ MPI) trên AWS EC2
*(Từng bước click chuột trên AWS Console & Ước tính chi phí chi tiết)*

Tài liệu này hướng dẫn chi tiết cách tạo các máy ảo AWS EC2 từ giao diện AWS Console, cấu hình mạng, thiết lập SSH không mật khẩu (Passwordless SSH), cài đặt môi trường, biên dịch/chạy mô phỏng song song N-Body trên cụm Cluster và bảng ước tính chi phí.

---

## PHẦN 1: Tạo các máy ảo EC2 trên AWS Console (Click-by-click)

Chúng ta cần tạo tổng cộng **5 máy ảo** (1 Master Node và 4 Compute Nodes). Để tiết kiệm thời gian, chúng ta sẽ tạo cùng một lúc 5 máy ảo rồi đổi tên sau.

### Bước 1.1: Truy cập trang quản trị EC2
1. Đăng nhập vào tài khoản [AWS Management Console](https://aws.amazon.com/console/).
2. Trên thanh tìm kiếm ở đỉnh màn hình (Search bar), gõ `EC2` và nhấn click vào dịch vụ **EC2** hiển thị đầu tiên.
3. Ở góc phải phía trên màn hình, chọn **Region** gần Việt Nam nhất để giảm độ trễ (Khuyên dùng: **Singapore** hoặc **Tokyo**).

### Bước 1.2: Cấu hình tạo máy ảo (Launch Instance)
1. Nhấn nút màu cam **Launch instance** ở giữa màn hình.
2. Tại mục **Name and tags**:
   * Nhập tên chung: `hpc-cluster` (Chúng ta sẽ đổi tên riêng sau).
3. Tại mục **Application and OS Images (Amazon Machine Image - AMI)**:
   * Click vào tab **Quick Start** (được chọn mặc định).
   * Click chọn logo của **Ubuntu**.
   * Tại dropdown **Amazon Machine Image (AMI)**, chọn **Ubuntu Server 22.04 LTS (HVM), SSD Volume Type** (loại 64-bit x86).
4. Tại mục **Instance type**:
   * Click chọn dòng máy ảo phù hợp.
   * *Lựa chọn 1 (Tiết kiệm)*: Chọn `t3.medium` (2 vCPU, 4 GiB RAM).
   * *Lựa chọn 2 (Hiệu năng cao)*: Chọn `c5.large` (2 vCPU, 4 GiB RAM - Tối ưu cho tính toán số học).
5. Tại mục **Key pair (login)**:
   * Click chọn **Create new key pair** nếu bạn chưa có.
   * **Key pair name**: Gõ `hpc-key`.
   * **Key pair type**: Chọn `RSA`.
   * **Private key file format**: Chọn `.pem` (dành cho OpenSSH/Linux/MacOS hoặc PowerShell trên Windows).
   * Nhấn nút **Create key pair**. Trình duyệt sẽ tự động tải về tệp tin `hpc-key.pem`. Hãy lưu tệp tin này vào thư mục an toàn trên máy của bạn (ví dụ: `C:\Users\Tên_User\.ssh\` hoặc thư mục Downloads).
6. Tại mục **Network settings**:
   * Click nút **Edit** ở góc phải trên của bảng Network settings.
   * **Firewall (security groups)**: Chọn **Create security group**.
   * **Security group name**: Nhập `hpc-security-group`.
   * **Description**: Nhập `SG for HPC Galaxy simulation cluster`.
   * Tại mục cấu hình **Inbound Security Group Rules** (Quy tắc kết nối vào):
     * Mặc định đã có sẵn Rule 1 cho SSH (Cổng 22). Tại dropdown **Source type**, chọn **My IP** (để chỉ có máy bạn mới SSH được vào, giúp bảo mật tối đa).
7. Tại mục **Configure storage**:
   * Mặc định là `8 GiB` gp3. Hãy đổi thành `15 GiB` để thoải mái lưu trữ dữ liệu hạt lớn và môi trường Python venv.
8. Tại mục **Summary** (Cột bên phải màn hình):
   * Nhập số lượng máy ảo cần tạo tại ô **Number of instances**: Nhập **`5`**.
   * Nhấn nút màu cam **Launch instance** ở dưới cùng.
   * Chờ khoảng 10-15 giây rồi nhấn **View all instances** ở cuối trang để quay lại danh sách máy ảo.

### Bước 1.3: Đổi tên riêng cho các Instances
Sau khi nhấn View all instances, bạn sẽ thấy 5 máy ảo đang ở trạng thái `Pending` (Đang khởi tạo). 
1. Click vào biểu tượng **Edit (hình bút chì)** ở cột *Name* của từng máy ảo và đặt lại tên lần lượt:
   * Máy ảo 1: `hpc-master`
   * Máy ảo 2: `hpc-node1`
   * Máy ảo 3: `hpc-node2`
   * Máy ảo 4: `hpc-node3`
   * Máy ảo 5: `hpc-node4`

---

## PHẦN 2: Cấu hình Security Group mở cổng giao tiếp cụm (Click-by-click)

Chúng ta cần cho phép:
* Cổng **8000** để Dashboard ở máy cá nhân gọi tới Backend API của Master.
* Mở **toàn bộ traffic nội bộ** giữa các node để MPI giao tiếp dữ liệu.

### Bước 2.1: Truy cập Security Group
1. Tại danh sách máy ảo EC2, click vào tên máy ảo `hpc-master`.
2. Cuộn xuống dưới màn hình, click vào tab **Security**.
3. Click vào liên kết của Security Group dưới dòng **Security groups** (có tên dạng `hpc-security-group` hoặc `sg-xxxxxxxx`).
4. Tại trang chi tiết Security Group, nhấn nút **Edit inbound rules** ở góc phải dưới của bảng Inbound Rules.

### Bước 2.2: Thêm luật mở cổng
1. Nhấn nút **Add rule** để thêm quy tắc mới (Rule 2):
   * **Type**: Chọn **Custom TCP**.
   * **Port range**: Nhập **`8000`**.
   * **Source**: Chọn **Anywhere-IPv4** (`0.0.0.0/0`) để Web Dashboard từ máy của bạn có thể gọi API vào máy Master.
2. Nhấn nút **Add rule** tiếp để thêm quy tắc thứ ba (Rule 3 - Quy tắc quan trọng nhất cho MPI):
   * **Type**: Chọn **All traffic** (Tất cả lưu lượng).
   * **Source**: Chọn **Custom**.
   * Ở ô nhập bên cạnh, gõ **`sg-`** hệ thống sẽ tự động hiển thị gợi ý ID của chính Security Group hiện tại của bạn. Click chọn nó (Ví dụ: `sg-0123456789abcdef`).
   * *Ý nghĩa*: Luật này cho phép tất cả các máy ảo có chung Security Group này được truyền nhận mọi dữ liệu thông qua tất cả các cổng mạng của nhau.
3. Nhấn nút **Save rules** ở góc phải dưới cùng.

---

## PHẦN 3: Thiết lập mạng & SSH nội bộ

Khi các máy ảo đã ở trạng thái `Running`, hãy ghi lại:
* **Public IP (IP công cộng)** của máy `hpc-master` (Dùng để kết nối API từ Web Dashboard máy cá nhân của bạn).
* **Private IP (IP nội bộ)** của tất cả 5 máy.

### Bước 3.1: Đổi quyền tệp khóa Private Key trên máy tính cá nhân
Mở terminal trên máy tính của bạn (PowerShell trên Windows hoặc Terminal trên macOS/Linux) và di chuyển vào thư mục chứa tệp `hpc-key.pem` đã tải về:
* **Đối với Windows (PowerShell)**:
  ```powershell
  # Đặt quyền chỉ đọc cho tệp khóa
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
*Nhấn `Ctrl + O`, tiếp tục nhấn `Enter` để lưu, sau đó nhấn `Ctrl + X` để thoát nano.*

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
   *Mở tệp `hpc-key.pem` trên máy cá nhân của bạn bằng Notepad/TextEdit, copy toàn bộ nội dung dán vào cửa sổ Terminal của máy Master. Nhấn `Ctrl + O`, `Enter` để lưu, `Ctrl + X` để thoát.*
   
4. Đặt quyền truy cập cho tệp khóa tạm thời trên Master:
   ```bash
   chmod 400 ~/.ssh/hpc-key.pem
   ```
5. Chạy các lệnh sau để tự động copy khóa công khai từ Master sang các Compute Node mà không cần bạn phải thao tác copy thủ công từng máy:
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

Bạn cần chạy lệnh cài đặt môi trường trên **CẢ 5 MÁYẢO** (Bạn có thể chạy song song trên nhiều tab Terminal hoặc chạy lần lượt):

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
2. Thiết lập đồng bộ hóa sang các Node tính toán (Chạy các lệnh rsync này từ máy Master):
   ```bash
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node1:/home/ubuntu/hpc-galaxy-sim/
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node2:/home/ubuntu/hpc-galaxy-sim/
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node3:/home/ubuntu/hpc-galaxy-sim/
   rsync -avz /home/ubuntu/hpc-galaxy-sim/ ubuntu@node4:/home/ubuntu/hpc-galaxy-sim/
   ```

### Bước 4.2: Biên dịch mã nguồn MPI
1. Trên máy Master, di chuyển vào thư mục dự án và chạy trình biên dịch `mpicxx`:
   ```bash
   cd /home/ubuntu/hpc-galaxy-sim
   mpicxx -O3 scripts/nbody_mpi.cpp -o scripts/nbody_mpi
   ```
2. Đồng bộ tệp nhị phân vừa biên dịch sang các Node để tránh phải biên dịch lại trên từng máy:
   ```bash
   for node in node1 node2 node3 node4; do
       rsync -avz /home/ubuntu/hpc-galaxy-sim/scripts/nbody_mpi ubuntu@$node:/home/ubuntu/hpc-galaxy-sim/scripts/nbody_mpi
   done
   ```

### Bước 4.3: Chạy ứng dụng FastAPI Backend (Chỉ trên máy Master)
1. Truy cập thư mục backend:
   ```bash
   cd /home/ubuntu/hpc-galaxy-sim/backend
   ```
2. Khởi tạo môi trường ảo Python và kích hoạt:
   ```bash
   python3 -m venv venv
   source venv/bin/activate
   ```
3. Cài đặt các thư viện cần thiết:
   ```bash
   pip install -r requirements.txt
   ```
4. Chạy Backend API ngầm để phục vụ Dashboard:
   ```bash
   nohup python3 run.py > uvicorn.log 2>&1 &
   ```
5. Đảm bảo backend đang chạy đúng bằng cách kiểm tra log:
   ```bash
   tail -n 10 uvicorn.log
   ```
   *(Nếu thấy dòng `Uvicorn running on http://0.0.0.0:8000` là thành công).*

---

## PHẦN 5: Ước tính chi phí vận hành cụm Cluster trong 1 giờ (AWS Singapore)

Dưới đây là bảng ước tính chi phí cho cụm 5 Instances chạy trong **1 giờ** (giá tính theo On-Demand Instance tại khu vực Singapore / Tokyo):

| Thành phần | Số lượng | Cấu hình kỹ thuật | Đơn giá / Giờ (USD) | Tổng chi phí / Giờ (USD) | Tổng chi phí / Giờ (VND) |
| :--- | :---: | :--- | :---: | :---: | :---: |
| **Máy ảo t3.medium** | 5 | 2 vCPUs, 4 GiB RAM | $0.0416 | **$0.208** | ~5.200 VND |
| **Ổ cứng SSD gp3** | 5 (15GB/máy) | 75 GB tổng dung lượng | $0.0001 | **$0.005** | ~120 VND |
| **Data Transfer** | - | Nhận dữ liệu (Free) / Xuất log nhẹ | $0.09 / GB | **$0.002** | ~50 VND |
| **TỔNG CỘNG (Mức Tiêu chuẩn)** | **5** | **Cụm 10 vCPUs, 20 GiB RAM** | | **$0.215** | **~5.370 VND** |

### Nếu nâng cấp lên máy ảo tối ưu tính toán (Compute Optimized - Khuyên dùng cho mô phỏng):

| Thành phần | Số lượng | Cấu hình kỹ thuật | Đơn giá / Giờ (USD) | Tổng chi phí / Giờ (USD) | Tổng chi phí / Giờ (VND) |
| :--- | :---: | :--- | :---: | :---: | :---: |
| **Máy ảo c5.large** | 5 | 2 vCPUs, 4 GiB (Compute) | $0.0850 | **$0.425** | ~10.600 VND |
| **Ổ cứng gp3 + Mạng** | 5 | 75 GB SSD | - | **$0.007** | ~170 VND |
| **TỔNG CỘNG (Mức Hiệu năng)** | **5** | **Cụm 10 vCPUs, 20 GiB RAM** | | **$0.432** | **~10.770 VND** |

> [!TIP]
> **Cách tiết kiệm chi phí tối đa**:
> 1. Khi dừng thí nghiệm/không sử dụng nữa, hãy truy cập AWS Console, chọn cả 5 instances, nhấn **Instance state** -> chọn **Terminate instance** (Xóa hoàn toàn máy ảo) hoặc **Stop instance** (Tạm dừng máy ảo, khi stop chỉ mất tiền lưu trữ SSD rất rẻ khoảng 150đ/máy/ngày, không mất tiền máy ảo chạy).
> 2. Nếu tài khoản AWS của bạn mới tạo (trong vòng 12 tháng), bạn sẽ được hưởng chính sách **AWS Free Tier** (Miễn phí 750 giờ chạy máy ảo `t2.micro` hoặc `t3.micro` hàng tháng). Tuy nhiên do RAM của `micro` hơi thấp (1-2GB), việc biên dịch hoặc sinh 100M hạt sẽ bị tràn bộ nhớ (Out of Memory). Bạn nên dùng `t3.medium` trở lên để chạy thử nghiệm ổn định.
