#!/bin/bash

# Script tự động cài đặt và triển khai cụm mô phỏng HPC N-Body (Master Node)
# Chạy script bằng quyền user ubuntu (không chạy bằng root trực tiếp, script tự dùng sudo khi cần)
# Lệnh chạy: chmod +x setup_all.sh && ./setup_all.sh

set -e # Dừng script nếu có bất kỳ lệnh nào bị lỗi

# Định nghĩa các mã màu để in log đẹp mắt
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}==================================================================${NC}"
echo -e "${BLUE}   BẮT ĐẦU TỰ ĐỘNG THIẾT LẬP VÀ BUILD CỤM HPC GALAXY SIMULATION   ${NC}"
echo -e "${BLUE}==================================================================${NC}"

# Đường dẫn dự án mặc định
PROJECT_DIR="/home/ubuntu/hpc-galaxy-sim"

if [ ! -d "$PROJECT_DIR" ]; then
    # Thử lấy đường dẫn hiện tại nếu không chạy ở thư mục mặc định
    PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
fi

echo -e "${YELLOW}[1/6] Đang cập nhật hệ thống và cài đặt các gói phụ trợ...${NC}"
sudo apt-get update
sudo apt-get install -y build-essential openmpi-bin openmpi-common libopenmpi-dev git \
                        python3-pip python3-venv python3-dev nginx rsync

echo -e "${GREEN}✓ Cài đặt các gói hệ thống thành công.${NC}"

echo -e "${YELLOW}[2/6] Đang tự động phát hiện các Compute Nodes cấu hình trong /etc/hosts...${NC}"
# Tìm các dòng chứa nodeX (ví dụ node1, node2, node3) trong file hosts
NODES=$(grep -oE "node[0-9]+" /etc/hosts | sort -u || true)

if [ -z "$NODES" ]; then
    echo -e "${RED}⚠ Cảnh báo: Không tìm thấy node tính toán nào (node1, node2, ...) trong /etc/hosts!${NC}"
    echo -e "${RED}Vui lòng cấu hình các node trong /etc/hosts trước nếu chạy cụm Multi-node.${NC}"
else
    echo -e "${GREEN}✓ Phát hiện các node: ${NODES//$'\n'/ }${NC}"
fi

echo -e "${YELLOW}[3/6] Đang biên dịch mã nguồn C++ MPI...${NC}"
cd "$PROJECT_DIR"
mpicxx -O3 scripts/nbody_mpi.cpp -o scripts/nbody_mpi
echo -e "${GREEN}✓ Biên dịch MPI thành công tại: scripts/nbody_mpi${NC}"

if [ ! -z "$NODES" ]; then
    echo -e "${YELLOW}Đang đồng bộ mã nguồn và tệp nhị phân sang các Compute Nodes...${NC}"
    for node in $NODES; do
        echo -e " -> Đang đồng bộ tới ${node}..."
        # Tạo thư mục đích trên node từ xa nếu chưa có
        ssh -o StrictHostKeyChecking=no ubuntu@$node "mkdir -p $PROJECT_DIR/scripts"
        # Đồng bộ file nhị phân vừa biên dịch
        rsync -avz "$PROJECT_DIR/scripts/nbody_mpi" ubuntu@$node:"$PROJECT_DIR/scripts/nbody_mpi"
    done
    echo -e "${GREEN}✓ Đồng bộ sang tất cả các Compute Nodes thành công.${NC}"
fi

echo -e "${YELLOW}[4/6] Cấu hình và triển khai Frontend (Web Server Nginx)...${NC}"
# Sao chép tệp index.html và .env vào thư mục root của Nginx
sudo mkdir -p /var/www/html
sudo cp "$PROJECT_DIR/frontend/index.html" /var/www/html/index.html

if [ -f "$PROJECT_DIR/frontend/.env" ]; then
    sudo cp "$PROJECT_DIR/frontend/.env" /var/www/html/.env
    echo -e " -> Đã sao chép tệp .env cấu hình Frontend."
else
    echo -e "${RED}⚠ Cảnh báo: Không tìm thấy tệp frontend/.env. Sẽ tạo tệp mặc định trỏ về IP Master.${NC}"
    # Lấy IP Public hoặc IP Private của Master làm API_URL mặc định nếu không có .env
    MASTER_IP=$(hostname -I | awk '{print $1}')
    echo "API_URL=http://$MASTER_IP" | sudo tee /var/www/html/.env
fi

# Ghi cấu hình Reverse Proxy cho Nginx
echo -e " -> Ghi cấu hình Nginx Reverse Proxy..."
sudo tee /etc/nginx/sites-available/default > /dev/null <<EOF
server {
    listen 80 default_server;
    listen [::]:80 default_server;

    root /var/www/html;
    index index.html;

    server_name _;

    # Phục vụ giao diện tĩnh Web Dashboard
    location / {
        try_files \$uri \$uri/ =404;
    }

    # Reverse Proxy chuyển tiếp yêu cầu API sang FastAPI Backend (Port 8000)
    location /api {
        proxy_pass http://127.0.0.1:8000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host \$host;
        proxy_cache_bypass \$http_upgrade;
    }
}
EOF

# Kiểm tra cú pháp cấu hình và restart Nginx
sudo nginx -t
sudo systemctl restart nginx
echo -e "${GREEN}✓ Cấu hình và khởi động lại Nginx thành công.${NC}"

echo -e "${YELLOW}[5/6] Thiết lập Python Virtual Environment cho Backend...${NC}"
cd "$PROJECT_DIR/backend"

# Nếu chưa có virtual env thì tạo mới
if [ ! -d "venv" ]; then
    python3 -m venv venv
    echo -e " -> Đã tạo Python Virtual Environment mới."
fi

# Kích hoạt venv và cài đặt thư viện
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt
echo -e "${GREEN}✓ Cài đặt dependencies Backend thành công.${NC}"

echo -e "${YELLOW}[6/6] Khởi chạy FastAPI Backend ngầm (Background Service)...${NC}"
# Kiểm tra xem có tiến trình backend cũ đang chạy không để giải phóng port 8000
PID=$(pgrep -f "run.py" || true)
if [ ! -z "$PID" ]; then
    echo -e " -> Đang tắt phiên bản Backend cũ đang chạy (PID: $PID)..."
    kill $PID || sudo kill -9 $PID
    sleep 2
fi

# Khởi chạy Backend ngầm bằng nohup
nohup python3 run.py > uvicorn.log 2>&1 &
echo -e "${GREEN}✓ Khởi chạy Backend ngầm thành công (PID: $!).${NC}"
echo -e "${YELLOW}Logs của Backend được ghi tại: $PROJECT_DIR/backend/uvicorn.log${NC}"

echo -e "${BLUE}==================================================================${NC}"
echo -e "${GREEN}        ĐÃ HOÀN THÀNH TOÀN BỘ CẤU HÌNH VÀ TRIỂN KHAI CỤM MẠNG!    ${NC}"
echo -e "${BLUE}==================================================================${NC}"
echo -e "${YELLOW}Địa chỉ truy cập Web: http://$(curl -s ifconfig.me || hostname -I | awk '{print $1}')${NC}"
echo -e "${BLUE}==================================================================${NC}"
