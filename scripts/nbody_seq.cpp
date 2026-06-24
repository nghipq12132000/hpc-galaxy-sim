#include <iostream>  // Thư viện nhập xuất chuẩn C++
#include <vector>    // Thư viện quản lý mảng động std::vector
#include <string>    // Thư viện xử lý chuỗi ký tự std::string
#include <cmath>     // Thư viện hàm toán học (sqrt, cos, sin,...)
#include <fstream>   // Thư viện đọc ghi file của C++
#include <sstream>   // Thư viện luồng chuỗi std::stringstream
#include <chrono>    // Thư viện đo thời gian chuẩn C++11
#include <cstdlib>   // Thư viện chuẩn C (rand, RAND_MAX,...)
#include <sys/time.h> // Thư viện đo thời gian hệ thống Unix/Linux

#define G 1.0        // Hằng số hấp dẫn tỷ lệ trong mô phỏng
#define EPSILON 0.8 // Hệ số làm mềm lực (softening factor) tránh chia cho 0 khi va chạm gần

// Cấu trúc hệ thống hạt sử dụng định dạng SoA (Structure of Arrays) để tối ưu hóa truy cập bộ nhớ cache
struct ParticleSystem {
    std::vector<double> x, y, z;    // Các mảng động lưu tọa độ x, y, z của hạt
    std::vector<double> vx, vy, vz; // Các mảng động lưu thành phần vận tốc vx, vy, vz
    std::vector<double> mass;       // Mảng động lưu khối lượng của từng hạt
    int n;                          // Tổng số lượng hạt trong hệ thống
};

// Hàm cấp phát bộ nhớ cho mảng động của hệ thống hạt
void allocate_system(ParticleSystem &sys, int n) {
    sys.n = n;               // Thiết lập tổng số lượng hạt
    sys.x.resize(n);         // Cấp phát kích thước n cho tọa độ x
    sys.y.resize(n);         // Cấp phát kích thước n cho tọa độ y
    sys.z.resize(n);         // Cấp phát kích thước n cho tọa độ z
    sys.vx.resize(n);        // Cấp phát kích thước n cho vận tốc vx
    sys.vy.resize(n);        // Cấp phát kích thước n cho vận tốc vy
    sys.vz.resize(n);        // Cấp phát kích thước n cho vận tốc vz
    sys.mass.resize(n);      // Cấp phát kích thước n cho khối lượng hạt
}

// Đọc điều kiện ban đầu của hệ hạt từ tệp văn bản dữ liệu
int read_input_file(const char *filename, ParticleSystem &sys) {
    std::ifstream file(filename); // Mở file dữ liệu đầu vào bằng luồng đọc ifstream
    if (!file.is_open()) {        // Kiểm tra xem file có mở thành công không
        std::cerr << "Lỗi: Không thể mở file đầu vào " << filename << std::endl;
        return 0;                 // Trả về 0 nếu mở file thất bại
    }

    int n = 0;                    // Biến đếm số lượng hạt (số dòng dữ liệu)
    std::string line;             // Chuỗi lưu tạm nội dung từng dòng của file
    while (std::getline(file, line)) { // Đọc từng dòng của file cho tới cuối
        if (!line.empty()) {      // Nếu dòng không trống thì tăng biến đếm số hạt
            n++;
        }
    }
    file.clear();                 // Xóa cờ trạng thái kết thúc file (EOF) của luồng đọc
    file.seekg(0, std::ios::beg); // Đưa con trỏ đọc file về lại đầu tệp tin

    allocate_system(sys, n);      // Cấp phát không gian bộ nhớ cho hệ hạt vừa đếm được

    for (int i = 0; i < n; i++) {  // Vòng lặp đọc thông số từng hạt từ file
        if (!(file >> sys.x[i] >> sys.y[i] >> sys.z[i] 
                   >> sys.vx[i] >> sys.vy[i] >> sys.vz[i] 
                   >> sys.mass[i])) { // Đọc 7 giá trị thực tương ứng cho mỗi dòng hạt
            std::cerr << "Lỗi: Định dạng dữ liệu không hợp lệ tại dòng " << i + 1 << std::endl;
            file.close();         // Đóng file
            return 0;             // Trả về lỗi định dạng thất bại
        }
    }

    file.close();                 // Đóng tệp dữ liệu sau khi đọc thành công
    return n;                     // Trả về tổng số hạt đã nạp vào hệ thống
}

// Tạo điều kiện ban đầu ngẫu nhiên nếu không sử dụng file đầu vào (Hệ thống đĩa Keplerian dự phòng)
void init_random_system(ParticleSystem &sys, int n) {
    allocate_system(sys, n);      // Khởi tạo và cấp phát bộ nhớ cho n hạt
    int half_n = n / 2;           // Phân nửa số hạt thuộc về thiên hà 1, nửa còn lại thuộc thiên hà 2

    for (int i = 0; i < n; i++) {  // Vòng lặp khởi tạo tọa độ và vận tốc ban đầu cho từng hạt
        double theta = ((double)rand() / RAND_MAX) * 2.0 * M_PI; // Sinh góc theta ngẫu nhiên từ 0 đến 2pi
        double r = 5.0 + ((double)rand() / RAND_MAX) * 25.0;     // Sinh bán kính r ngẫu nhiên từ 5 đến 30 đơn vị
        
        if (i < half_n) {         // Cài đặt các thông số cho hạt thuộc Thiên hà 1 (nằm bên trái)
            sys.x[i] = -80.0 + r * cos(theta); // Tọa độ x lệch về tâm -80
            sys.y[i] = r * sin(theta);         // Tọa độ y xoay tròn quanh tâm
            sys.z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0; // Tọa độ z (độ dày đĩa ngẫu nhiên)
            
            double v_orbit = sqrt(G * 22000.0 / r); // Tính vận tốc quỹ đạo Keplerian v = sqrt(G*M/r)
            sys.vx[i] = 2.0 - v_orbit * sin(theta); // Vận tốc vx kèm theo vận tốc tịnh tiến thiên hà chậm hơn
            sys.vy[i] = 0.4 + v_orbit * cos(theta); // Vận tốc vy kèm theo vận tốc tịnh tiến thiên hà chậm hơn
            sys.vz[i] = 0.0;                        // Thành phần vận tốc vz ban đầu bằng 0
            sys.mass[i] = (i == 0) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9; // Hạt 0 là lỗ đen siêu nặng
        } else {                  // Cài đặt các thông số cho hạt thuộc Thiên hà 2 (nằm bên phải)
            sys.x[i] = 80.0 + r * cos(theta);  // Tọa độ x lệch về tâm +80
            sys.y[i] = r * sin(theta);         // Tọa độ y xoay tròn quanh tâm
            sys.z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0; // Tọa độ z (độ dày đĩa ngẫu nhiên)
            
            double v_orbit = sqrt(G * 22000.0 / r); // Tính vận tốc quỹ đạo Keplerian v = sqrt(G*M/r)
            sys.vx[i] = -2.0 + v_orbit * sin(theta); // Vận tốc vx kèm theo vận tốc tịnh tiến ngược chiều chậm hơn
            sys.vy[i] = -0.4 - v_orbit * cos(theta); // Vận tốc vy kèm theo vận tốc tịnh tiến ngược chiều chậm hơn
            sys.vz[i] = 0.0;                         // Thành phần vận tốc vz ban đầu bằng 0
            sys.mass[i] = (i == half_n) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9; // Hạt half_n là lỗ đen 2
        }
    }
}

// Cấu trúc nút cho cây 3D Octree sử dụng thuật toán Barnes-Hut
struct OctreeNode {
    double cx, cy, cz;      // Tọa độ tâm khối lượng (Center of Mass)
    double total_mass;      // Tổng khối lượng các hạt thuộc nút này
    double center_x, center_y, center_z; // Tâm hình học của hộp bao (Bounding Box)
    double size;            // Kích thước cạnh hộp bao
    int particle_idx;       // Chỉ số của hạt nếu là nút lá (>=0), -1 nếu trống, -2 nếu là nút trong (internal node)
    int children[8];        // Chỉ số của 8 nút con trong mảng phẳng (-1 nếu không có)
};

// Cấu trúc cây 3D Octree
struct Octree {
    std::vector<OctreeNode> nodes;
    const std::vector<double> &px;
    const std::vector<double> &py;
    const std::vector<double> &pz;
    const std::vector<double> &pmass;

    Octree(const std::vector<double> &x, const std::vector<double> &y, const std::vector<double> &z, const std::vector<double> &mass)
        : px(x), py(y), pz(z), pmass(mass) {
        nodes.reserve(8 * x.size());
    }

    // Xây dựng cây từ tất cả hạt
    void build() {
        nodes.clear();
        int n = px.size();
        if (n == 0) return;

        // 1. Tìm bounding box bao quanh tất cả các hạt
        double min_x = px[0], max_x = px[0];
        double min_y = py[0], max_y = py[0];
        double min_z = pz[0], max_z = pz[0];

        for (int i = 1; i < n; i++) {
            if (px[i] < min_x) min_x = px[i];
            if (px[i] > max_x) max_x = px[i];
            if (py[i] < min_y) min_y = py[i];
            if (py[i] > max_y) max_y = py[i];
            if (pz[i] < min_z) min_z = pz[i];
            if (pz[i] > max_z) max_z = pz[i];
        }

        double center_x = 0.5 * (min_x + max_x);
        double center_y = 0.5 * (min_y + max_y);
        double center_z = 0.5 * (min_z + max_z);
        double size = max_x - min_x;
        if (max_y - min_y > size) size = max_y - min_y;
        if (max_z - min_z > size) size = max_z - min_z;
        if (size < 1e-6) size = 1e-6; // Tránh trường hợp kích thước bằng 0

        // Tạo nút gốc (root)
        OctreeNode root;
        root.center_x = center_x;
        root.center_y = center_y;
        root.center_z = center_z;
        root.size = size;
        root.cx = 0.0;
        root.cy = 0.0;
        root.cz = 0.0;
        root.total_mass = 0.0;
        root.particle_idx = -1;
        for (int i = 0; i < 8; i++) root.children[i] = -1;

        nodes.push_back(root);

        // Chèn tất cả các hạt vào cây
        for (int i = 0; i < n; i++) {
            insert(0, i);
        }

        // Tính toán tâm khối lượng cho tất cả các nút trong
        compute_distribution(0);
    }

    // Xác định góc phần tư con tương ứng của hạt
    int get_child_index(const OctreeNode &node, double x, double y, double z) {
        int idx = 0;
        if (x >= node.center_x) idx |= 1;
        if (y >= node.center_y) idx |= 2;
        if (z >= node.center_z) idx |= 4;
        return idx;
    }

    // Đệ quy chèn hạt vào cây
    void insert(int node_idx, int p_idx) {
        double x_val = px[p_idx];
        double y_val = py[p_idx];
        double z_val = pz[p_idx];
        double m_val = pmass[p_idx];

        // Nếu nút đang trống, chèn trực tiếp hạt vào đây
        if (nodes[node_idx].particle_idx == -1) {
            nodes[node_idx].particle_idx = p_idx;
            nodes[node_idx].cx = x_val;
            nodes[node_idx].cy = y_val;
            nodes[node_idx].cz = z_val;
            nodes[node_idx].total_mass = m_val;
            return;
        }

        // Nếu nút là nút lá chứa một hạt khác, tiến hành phân chia (subdivide)
        if (nodes[node_idx].particle_idx >= 0) {
            int existing_p_idx = nodes[node_idx].particle_idx;

            // Nếu 2 hạt trùng khít tọa độ, tạo độ lệch siêu nhỏ để tránh đệ quy vô hạn
            if (std::abs(px[existing_p_idx] - x_val) < 1e-9 &&
                std::abs(py[existing_p_idx] - y_val) < 1e-9 &&
                std::abs(pz[existing_p_idx] - z_val) < 1e-9) {
                x_val += 1e-8 * (((double)rand() / RAND_MAX) - 0.5);
                y_val += 1e-8 * (((double)rand() / RAND_MAX) - 0.5);
                z_val += 1e-8 * (((double)rand() / RAND_MAX) - 0.5);
            }

            double parent_size = nodes[node_idx].size;
            double child_size = 0.5 * parent_size;
            double parent_cx = nodes[node_idx].center_x;
            double parent_cy = nodes[node_idx].center_y;
            double parent_cz = nodes[node_idx].center_z;

            int child_start_idx = nodes.size();
            for (int i = 0; i < 8; i++) {
                OctreeNode child;
                child.center_x = parent_cx + ((i & 1) ? 0.25 : -0.25) * parent_size;
                child.center_y = parent_cy + ((i & 2) ? 0.25 : -0.25) * parent_size;
                child.center_z = parent_cz + ((i & 4) ? 0.25 : -0.25) * parent_size;
                child.size = child_size;
                child.cx = 0.0;
                child.cy = 0.0;
                child.cz = 0.0;
                child.total_mass = 0.0;
                child.particle_idx = -1;
                for (int j = 0; j < 8; j++) child.children[j] = -1;
                nodes.push_back(child);
            }

            for (int i = 0; i < 8; i++) {
                nodes[node_idx].children[i] = child_start_idx + i;
            }

            nodes[node_idx].particle_idx = -2; // Đánh dấu thành nút trong

            // Chèn lại hạt cũ xuống các nút con
            int child_idx_existing = get_child_index(nodes[node_idx], px[existing_p_idx], py[existing_p_idx], pz[existing_p_idx]);
            insert(nodes[node_idx].children[child_idx_existing], existing_p_idx);

            // Chèn hạt mới
            int child_idx_new = get_child_index(nodes[node_idx], x_val, y_val, z_val);
            insert(nodes[node_idx].children[child_idx_new], p_idx);
            return;
        }

        // Nếu là nút trong, chèn tiếp đệ quy
        if (nodes[node_idx].particle_idx == -2) {
            int child_idx = get_child_index(nodes[node_idx], x_val, y_val, z_val);
            insert(nodes[node_idx].children[child_idx], p_idx);
            return;
        }
    }

    // Đệ quy tính tổng khối lượng và trọng tâm khối lượng của các nút trong
    void compute_distribution(int node_idx) {
        if (nodes[node_idx].particle_idx == -1) return;
        if (nodes[node_idx].particle_idx >= 0) return; // Nút lá đã có sẵn cx, cy, cz, mass

        double total_m = 0.0;
        double sum_cx = 0.0;
        double sum_cy = 0.0;
        double sum_cz = 0.0;

        for (int i = 0; i < 8; i++) {
            int child_idx = nodes[node_idx].children[i];
            if (child_idx != -1) {
                compute_distribution(child_idx);
                double child_m = nodes[child_idx].total_mass;
                if (child_m > 0) {
                    total_m += child_m;
                    sum_cx += nodes[child_idx].cx * child_m;
                    sum_cy += nodes[child_idx].cy * child_m;
                    sum_cz += nodes[child_idx].cz * child_m;
                }
            }
        }

        if (total_m > 0) {
            nodes[node_idx].total_mass = total_m;
            nodes[node_idx].cx = sum_cx / total_m;
            nodes[node_idx].cy = sum_cy / total_m;
            nodes[node_idx].cz = sum_cz / total_m;
        }
    }

    // Đệ quy tính gia tốc từ cây tác động lên hạt i
    void calculate_force(int node_idx, double ipx, double ipy, double ipz, int ip_idx, double theta, double epsilon, double G_const, double &ax, double &ay, double &az) const {
        if (nodes[node_idx].particle_idx == -1) return;

        // Nếu là nút lá, tính lực tương tác trực tiếp
        if (nodes[node_idx].particle_idx >= 0) {
            int jp_idx = nodes[node_idx].particle_idx;
            if (jp_idx == ip_idx) return;

            double dx = px[jp_idx] - ipx;
            double dy = py[jp_idx] - ipy;
            double dz = pz[jp_idx] - ipz;

            double distSqr = dx*dx + dy*dy + dz*dz + epsilon*epsilon;
            double invDist = 1.0 / std::sqrt(distSqr);
            double invDistCube = invDist * invDist * invDist;

            double f = G_const * pmass[jp_idx] * invDistCube;
            ax += dx * f;
            ay += dy * f;
            az += dz * f;
            return;
        }

        // Nếu là nút trong, kiểm tra tiêu chí mở rộng Multipole Acceptance Criterion (MAC)
        double dx = nodes[node_idx].cx - ipx;
        double dy = nodes[node_idx].cy - ipy;
        double dz = nodes[node_idx].cz - ipz;

        double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (dist == 0.0) return;

        // Nếu kích thước nút chia cho khoảng cách < theta, xấp xỉ lực bằng Center of Mass
        if (nodes[node_idx].size / dist < theta) {
            double distSqr = dx*dx + dy*dy + dz*dz + epsilon*epsilon;
            double invDist = 1.0 / std::sqrt(distSqr);
            double invDistCube = invDist * invDist * invDist;

            double f = G_const * nodes[node_idx].total_mass * invDistCube;
            ax += dx * f;
            ay += dy * f;
            az += dz * f;
        } else {
            // Ngược lại, duyệt đệ quy tiếp các nút con
            for (int i = 0; i < 8; i++) {
                int child_idx = nodes[node_idx].children[i];
                if (child_idx != -1) {
                    calculate_force(child_idx, ipx, ipy, ipz, ip_idx, theta, epsilon, G_const, ax, ay, az);
                }
            }
        }
    }
};

// Tính toán lực bằng thuật toán Barnes-Hut O(N log N) và cập nhật Leapfrog
void update_particles(ParticleSystem &sys, double dt) {
    int n = sys.n;
    
    std::vector<double> ax(n, 0.0);
    std::vector<double> ay(n, 0.0);
    std::vector<double> az(n, 0.0);

    // 1. Khởi tạo và xây dựng cây Octree từ tất cả hạt toàn cục
    Octree tree(sys.x, sys.y, sys.z, sys.mass);
    tree.build();

    // 2. Tính toán gia tốc bằng cách duyệt cây song song OpenMP
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        double acc_x = 0.0;
        double acc_y = 0.0;
        double acc_z = 0.0;
        
        // Sử dụng theta = 0.5 cho cân bằng tốt nhất giữa hiệu năng và độ chính xác
        tree.calculate_force(0, sys.x[i], sys.y[i], sys.z[i], i, 0.5, EPSILON, G, acc_x, acc_y, acc_z);
        
        ax[i] = acc_x;
        ay[i] = acc_y;
        az[i] = acc_z;
    }

    // 3. Cập nhật vận tốc và tọa độ hạt theo phương pháp tích phân Leapfrog
    for (int i = 0; i < n; i++) {
        sys.vx[i] += ax[i] * dt;
        sys.vy[i] += ay[i] * dt;
        sys.vz[i] += az[i] * dt;
        
        sys.x[i] += sys.vx[i] * dt;
        sys.y[i] += sys.vy[i] * dt;
        sys.z[i] += sys.vz[i] * dt;
    }
}

// Đo thời gian hệ thống tính bằng giây
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL); // Lấy thời gian từ đồng hồ hệ thống
    return tv.tv_sec + tv.tv_usec * 1e-6; // Chuyển sang giá trị giây kiểu double
}

int main(int argc, char **argv) {
    if (argc < 4) { // Kiểm tra đối số dòng lệnh đầu vào
        std::cout << "Sử dụng: " << argv[0] << " <input_file_or_N> <steps> <dt>" << std::endl;
        std::cout << "Ví dụ 1 (đọc file): " << argv[0] << " ../Data/galaxy_collision_10k.txt 100 0.01" << std::endl;
        std::cout << "Ví dụ 2 (tự động tạo): " << argv[0] << " 10000 100 0.01" << std::endl;
        return 1;
    }

    ParticleSystem sys;  // Khai báo cấu trúc hệ thống hạt
    int n = 0;           // Số lượng hạt ban đầu
    int steps = atoi(argv[2]);  // Số bước nhảy thời gian mô phỏng
    double dt = atof(argv[3]);  // Độ dài bước thời gian tích phân

    char *endptr;
    int parsed_n = strtol(argv[1], &endptr, 10); // Thử phân tách tham số đầu tiên thành số nguyên

    double start_init = get_time(); // Đo mốc bắt đầu khởi tạo
    if (*endptr == '\0') { // Nếu tham số thứ nhất hoàn toàn là số nguyên thì sinh ngẫu nhiên
        n = parsed_n;
        std::cout << "Khởi tạo hệ thống ngẫu nhiên với " << n << " hạt..." << std::endl;
        init_random_system(sys, n); // Thực thi sinh dữ liệu đĩa Keplerian ngẫu nhiên
    } else { // Nếu là một đường dẫn tệp tin thì tiến hành đọc dữ liệu từ file
        const char *filename = argv[1];
        std::cout << "Đang đọc điều kiện ban đầu từ file: " << filename << "..." << std::endl;
        n = read_input_file(filename, sys); // Đọc tọa độ từ tệp tin
        if (n == 0) { // Thoát chương trình nếu đọc file lỗi
            return 1;
        }
        std::cout << "Nạp thành công " << n << " hạt." << std::endl;
    }
    double end_init = get_time(); // Đo mốc kết thúc khởi tạo
    std::cout << "Thời gian khởi tạo hệ thống: " << (end_init - start_init) << " giây." << std::endl;

    std::cout << "Bắt đầu chạy mô phỏng: " << steps << " bước, dt = " << dt << "..." << std::endl;
    double start_sim = get_time(); // Đo mốc thời gian bắt đầu chạy vòng lặp mô phỏng
    for (int step = 0; step < steps; step++) { // Thực thi các bước mô phỏng
        update_particles(sys, dt); // Gọi hàm cập nhật lực và động học
        if ((step + 1) % 10 == 0 || step == 0) { // Định kỳ hiển thị tiến độ mô phỏng
            std::cout << "Hoàn thành bước mô phỏng " << step + 1 << "/" << steps << "..." << std::endl;
        }
    }
    double end_sim = get_time(); // Đo mốc kết thúc mô phỏng
    
    double elapsed_sim = end_sim - start_sim; // Tính tổng thời gian mô phỏng thực tế
    std::cout << "\nMô phỏng kết thúc." << std::endl;
    std::cout << "Tổng thời gian chạy mô phỏng: " << elapsed_sim << " giây." << std::endl;
    // Tính toán hiệu năng quy đổi ra Mflops/sec
    std::cout << "Hiệu năng: " << ((double)steps * n * n * 20.0 / (elapsed_sim * 1e6)) << " Mflops/sec quy đổi" << std::endl;

    // Hiển thị tọa độ vị trí tâm của 2 thiên hà ở cuối mô phỏng để kiểm tra tính đúng đắn
    std::cout << "\nTọa độ cuối cùng của Tâm Thiên hà 1 (Hạt số 0): (" 
              << sys.x[0] << ", " << sys.y[0] << ", " << sys.z[0] << ")" << std::endl;
    if (n > n/2) {
        std::cout << "Tọa độ cuối cùng của Tâm Thiên hà 2 (Hạt số " << n/2 << "): (" 
                  << sys.x[n/2] << ", " << sys.y[n/2] << ", " << sys.z[n/2] << ")" << std::endl;
    }

    return 0; // Kết thúc chương trình
}
