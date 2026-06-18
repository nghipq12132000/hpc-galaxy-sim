#include <iostream>   // Thư viện nhập xuất chuẩn C++
#include <vector>     // Thư viện quản lý mảng động std::vector của C++
#include <string>     // Thư viện xử lý chuỗi ký tự std::string
#include <cmath>      // Thư viện các hàm toán học chuẩn (sqrt, cos, sin,...)
#include <fstream>    // Thư viện luồng đọc ghi tệp tin
#include <cstring>    // Thư viện sao chép vùng nhớ memcpy từ C
#include <cstdlib>    // Thư viện chuẩn C (rand, RAND_MAX,...)
#include <sys/time.h>  // Thư viện đo thời gian hệ thống Unix/Linux
#include <mpi.h>      // Thư viện MPI cho tính toán song song phân tán

#define G 1.0         // Hằng số hấp dẫn tỷ lệ dùng trong mô phỏng
#define EPSILON 0.8  // Hệ số làm mềm lực (softening factor) để tránh chia cho 0 khi các hạt tiến quá sát nhau

// Hàm đo thời gian hệ thống trả về thời gian dạng giây kiểu thực double
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL); // Lấy thời gian hiện tại của hệ thống
    return tv.tv_sec + tv.tv_usec * 1e-6; // Cộng phần giây và phần micro-giây chuyển về giây
}

int main(int argc, char **argv) {
    int rank, size;
    // Khởi tạo môi trường MPI
    MPI_Init(&argc, &argv);
    // Lấy ID (rank) của tiến trình hiện tại trong giao tiếp chung COMM_WORLD
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // Lấy tổng số lượng tiến trình (size) tham gia tính toán song song
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Kiểm tra tính đầy đủ của các đối số dòng lệnh ở Rank 0
    if (argc < 4 && rank == 0) {
        std::cout << "Sử dụng: mpirun -np <procs> " << argv[0] << " <input_file_or_N> <steps> <dt>" << std::endl;
        std::cout << "Ví dụ 1 (đọc file): mpirun -np 4 " << argv[0] << " ../Data/galaxy_collision_10k.txt 100 0.01" << std::endl;
        std::cout << "Ví dụ 2 (tự tạo): mpirun -np 4 " << argv[0] << " 10000 100 0.01" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1); // Buộc kết thúc sớm tất cả các tiến trình MPI do thiếu đối số
    }

    int n = 0;              // Tổng số lượng hạt toàn cục
    int steps = 0;          // Tổng số bước mô phỏng
    double dt = 0.0;        // Độ dài bước nhảy thời gian tích phân
    
    // Rank 0 đọc các đối số và phát quảng bá thông số cho tất cả các rank khác
    if (rank == 0) {
        steps = atoi(argv[2]); // Chuyển chuỗi đối số số bước sang kiểu số nguyên
        dt = atof(argv[3]);    // Chuyển chuỗi đối số dt sang kiểu số thực
    }

    // Phát quảng bá (Broadcast) các giá trị steps và dt từ Rank 0 tới toàn bộ các rank khác
    MPI_Bcast(&steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&dt, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Khai báo các mảng động toàn cục chứa dữ liệu hạt trên tiến trình Master (Rank 0)
    std::vector<double> master_x;
    std::vector<double> master_y;
    std::vector<double> master_z;
    std::vector<double> master_vx;
    std::vector<double> master_vy;
    std::vector<double> master_vz;
    std::vector<double> master_mass;

    double start_init = 0.0;
    if (rank == 0) {
        start_init = get_time(); // Ghi nhận thời gian bắt đầu khởi tạo dữ liệu
        char *endptr;
        int parsed_n = strtol(argv[1], &endptr, 10); // Thử tách đối số đầu tiên thành số nguyên
        
        if (*endptr == '\0') {
            // Nếu phân tách hoàn toàn là số nguyên thì tự sinh dữ liệu đĩa Keplerian ngẫu nhiên
            n = parsed_n;
            master_x.resize(n); master_y.resize(n); master_z.resize(n);
            master_vx.resize(n); master_vy.resize(n); master_vz.resize(n);
            master_mass.resize(n);
            
            int half_n = n / 2; // Chia đôi số lượng hạt cho 2 đĩa thiên hà
            for (int i = 0; i < n; i++) {
                double theta = ((double)rand() / RAND_MAX) * 2.0 * M_PI; // Sinh góc theta ngẫu nhiên từ 0 đến 2pi
                double r = 5.0 + ((double)rand() / RAND_MAX) * 25.0;     // Sinh bán kính r ngẫu nhiên từ 5 đến 30
                
                if (i < half_n) { // Các hạt thuộc Thiên hà 1 (trái)
                    master_x[i] = -80.0 + r * cos(theta);
                    master_y[i] = r * sin(theta);
                    master_z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
                    
                    double v_orbit = sqrt(G * 22000.0 / r); // Vận tốc quỹ đạo Keplerian tròn
                    master_vx[i] = 2.0 - v_orbit * sin(theta); // Cộng tịnh tiến vận tốc thiên hà chậm hơn
                    master_vy[i] = 0.4 + v_orbit * cos(theta);
                    master_vz[i] = 0.0;
                    master_mass[i] = (i == 0) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9;
                } else { // Các hạt thuộc Thiên hà 2 (phải)
                    master_x[i] = 80.0 + r * cos(theta);
                    master_y[i] = r * sin(theta);
                    master_z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
                    
                    double v_orbit = sqrt(G * 22000.0 / r);
                    master_vx[i] = -2.0 + v_orbit * sin(theta);
                    master_vy[i] = -0.4 - v_orbit * cos(theta);
                    master_vz[i] = 0.0;
                    master_mass[i] = (i == half_n) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9;
                }
            }
        } else {
            // Nếu đối số thứ nhất không phải là số nguyên thì đọc từ tệp tin dữ liệu đầu vào
            const char *filename = argv[1];
            std::ifstream file(filename); // Mở luồng đọc file
            if (!file.is_open()) {
                std::cerr << "Lỗi: Không thể mở file đầu vào " << filename << std::endl;
                MPI_Abort(MPI_COMM_WORLD, 1); // Hủy toàn bộ tiến trình MPI
            }
            
            std::string line;
            while (std::getline(file, line)) { // Đếm số lượng dòng trong file để tìm số hạt n
                if (!line.empty()) {
                    n++;
                }
            }
            file.clear(); // Xóa trạng thái kết thúc file
            file.seekg(0, std::ios::beg); // Quay lại đầu file
            
            master_x.resize(n); master_y.resize(n); master_z.resize(n);
            master_vx.resize(n); master_vy.resize(n); master_vz.resize(n);
            master_mass.resize(n);
            
            for (int i = 0; i < n; i++) { // Vòng lặp đọc 7 cột dữ liệu tọa độ, vận tốc và khối lượng
                if (!(file >> master_x[i] >> master_y[i] >> master_z[i]
                           >> master_vx[i] >> master_vy[i] >> master_vz[i]
                           >> master_mass[i])) {
                    std::cerr << "Lỗi: Định dạng dữ liệu không hợp lệ tại dòng " << i + 1 << std::endl;
                    file.close();
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
            file.close(); // Đóng file
        }
    }

    // Phát quảng bá tổng số lượng hạt N từ Rank 0 tới tất cả các tiến trình khác
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Tính toán phân chia hạt cho các tiến trình (Data decomposition)
    std::vector<int> sendcounts(size); // Mảng lưu số lượng hạt phân phối cho từng rank
    std::vector<int> displs(size);     // Mảng lưu vị trí bắt đầu lệch (offset) của từng rank trong mảng toàn cục
    
    int remainder = n % size; // Số hạt dư khi chia đều
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sendcounts[i] = n / size + (i < remainder ? 1 : 0); // Phân chia đều hạt, phân phối phần dư cho các rank đầu
        displs[i] = sum; // Xác định vị trí bắt đầu của rank tương ứng
        sum += sendcounts[i]; // Tăng dần vị trí lệch cho rank kế tiếp
    }
    
    int n_local = sendcounts[rank]; // Số lượng hạt cục bộ mà rank này chịu trách nhiệm xử lý
    int start_idx = displs[rank];   // Chỉ số bắt đầu toàn cục của các hạt cục bộ thuộc rank này

    // Cấp phát các mảng cục bộ lưu tọa độ, vận tốc và khối lượng của phân hoạch hạt thuộc rank này
    std::vector<double> local_x(n_local);
    std::vector<double> local_y(n_local);
    std::vector<double> local_z(n_local);
    std::vector<double> local_vx(n_local);
    std::vector<double> local_vy(n_local);
    std::vector<double> local_vz(n_local);
    std::vector<double> local_mass(n_local);

    // Phân tán (Scatterv) dữ liệu tọa độ ban đầu từ Master (Rank 0) tới tất cả các rank
    MPI_Scatterv(master_x.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, local_x.data(), n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_y.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, local_y.data(), n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_z.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, local_z.data(), n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_vx.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, local_vx.data(), n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_vy.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, local_vy.data(), n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_vz.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, local_vz.data(), n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_mass.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, local_mass.data(), n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Cấp phát mảng toàn cục trên mỗi rank để chứa tọa độ đồng bộ của tất cả các hạt từ mọi rank
    std::vector<double> global_x(n);
    std::vector<double> global_y(n);
    std::vector<double> global_z(n);
    std::vector<double> global_mass(n);

    // Sao chép khối lượng từ Master hoặc nhận phát quảng bá vì khối lượng không thay đổi trong suốt mô phỏng
    if (rank == 0) {
        std::memcpy(global_mass.data(), master_mass.data(), n * sizeof(double));
    }
    MPI_Bcast(global_mass.data(), n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double end_init = get_time(); // Đo mốc kết thúc khởi tạo và phân tán dữ liệu
        std::cout << "Thời gian khởi tạo và phân tán dữ liệu: " << (end_init - start_init) << " giây." << std::endl;
        std::cout << "Bắt đầu mô phỏng: " << steps << " bước, dt = " << dt << " trên " << size << " tiến trình..." << std::endl;
    }

    // Cấp phát mảng chứa thành phần gia tốc cục bộ của các hạt thuộc rank này
    std::vector<double> ax(n_local);
    std::vector<double> ay(n_local);
    std::vector<double> az(n_local);

    MPI_Barrier(MPI_COMM_WORLD); // Đồng bộ rào cản tất cả các tiến trình trước khi bắt đầu mô phỏng thực sự
    double start_sim = get_time(); // Đo mốc bắt đầu thời gian tính toán mô phỏng chính

    for (int step = 0; step < steps; step++) {
        // 1. Thu thập toàn bộ tọa độ x, y, z hiện tại từ mọi tiến trình về tất cả các tiến trình (Allgatherv)
        MPI_Allgatherv(local_x.data(), n_local, MPI_DOUBLE, global_x.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, MPI_COMM_WORLD);
        MPI_Allgatherv(local_y.data(), n_local, MPI_DOUBLE, global_y.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, MPI_COMM_WORLD);
        MPI_Allgatherv(local_z.data(), n_local, MPI_DOUBLE, global_z.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, MPI_COMM_WORLD);

        // 2. Tính toán gia tốc cục bộ cho phân hoạch hạt của rank hiện tại
        for (int i = 0; i < n_local; i++) {
            double acc_x = 0.0;
            double acc_y = 0.0;
            double acc_z = 0.0;
            
            double lx = local_x[i];
            double ly = local_y[i];
            double lz = local_z[i];
            
            int global_i = start_idx + i; // Quy đổi chỉ số hạt cục bộ sang chỉ số hạt toàn cục tương ứng

            // Vòng lặp tính lực tương tác hấp dẫn O(N) với toàn bộ các hạt trong hệ
            for (int j = 0; j < n; j++) {
                if (global_i == j) continue; // Bỏ qua tương tác xuyên tâm tự thân
                
                double dx = global_x[j] - lx; // Khoảng cách thành phần delta x
                double dy = global_y[j] - ly; // Khoảng cách thành phần delta y
                double dz = global_z[j] - lz; // Khoảng cách thành phần delta z
                
                double distSqr = dx*dx + dy*dy + dz*dz + EPSILON*EPSILON; // Bình phương khoảng cách cộng hệ số làm mềm
                double invDist = 1.0 / sqrt(distSqr); // Nghịch đảo khoảng cách 1/r
                double invDistCube = invDist * invDist * invDist; // Nghịch đảo khoảng cách lập phương 1/r^3
                
                double f = G * global_mass[j] * invDistCube; // Tính hệ số tương tác hấp dẫn Newton
                
                acc_x += dx * f; // Tích lũy gia tốc thành phần x
                acc_y += dy * f; // Tích lũy gia tốc thành phần y
                acc_z += dz * f; // Tích lũy gia tốc thành phần z
            }
            ax[i] = acc_x; // Lưu gia tốc cục bộ x
            ay[i] = acc_y; // Lưu gia tốc cục bộ y
            az[i] = acc_z; // Lưu gia tốc cục bộ z
        }

        // 3. Cập nhật vận tốc và tọa độ cục bộ sử dụng phương pháp tích phân Leapfrog (Verlet)
        for (int i = 0; i < n_local; i++) {
            local_vx[i] += ax[i] * dt; // Cập nhật vận tốc cục bộ vx = vx + ax * dt
            local_vy[i] += ay[i] * dt; // Cập nhật vận tốc cục bộ vy = vy + ay * dt
            local_vz[i] += az[i] * dt; // Cập nhật vận tốc cục bộ vz = vz + az * dt
            
            local_x[i] += local_vx[i] * dt; // Cập nhật tọa độ cục bộ x = x + vx * dt
            local_y[i] += local_vy[i] * dt; // Cập nhật tọa độ cục bộ y = y + vy * dt
            local_z[i] += local_vz[i] * dt; // Cập nhật tọa độ cục bộ z = z + vz * dt
        }

        // Tiến trình Rank 0 in tiến độ mô phỏng ra màn hình
        if (rank == 0 && ((step + 1) % 10 == 0 || step == 0)) {
            std::cout << "Hoàn thành bước mô phỏng " << step + 1 << "/" << steps << "..." << std::endl;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD); // Đồng bộ tất cả các tiến trình trước khi đo thời gian kết thúc mô phỏng
    double end_sim = get_time(); // Đo mốc kết thúc mô phỏng

    // Thu thập kết quả tọa độ cuối cùng của toàn bộ các hạt về tiến trình Master (Rank 0)
    std::vector<double> final_x;
    std::vector<double> final_y;
    std::vector<double> final_z;
    if (rank == 0) {
        final_x.resize(n);
        final_y.resize(n);
        final_z.resize(n);
    }

    // Thu thập dữ liệu (Gatherv) tọa độ x, y, z cục bộ từ các rank về mảng toàn cục của Rank 0
    MPI_Gatherv(local_x.data(), n_local, MPI_DOUBLE, final_x.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gatherv(local_y.data(), n_local, MPI_DOUBLE, final_y.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gatherv(local_z.data(), n_local, MPI_DOUBLE, final_z.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Tiến trình Master (Rank 0) hiển thị kết quả tổng hợp hiệu năng và tọa độ tâm thiên hà để xác minh tính chính xác
    if (rank == 0) {
        double elapsed_sim = end_sim - start_sim; // Tính tổng thời gian mô phỏng chính thức
        std::cout << "\nMô phỏng kết thúc." << std::endl;
        std::cout << "Tổng thời gian chạy mô phỏng: " << elapsed_sim << " giây." << std::endl;
        std::cout << "Hiệu năng: " << ((double)steps * n * n * 20.0 / (elapsed_sim * 1e6)) << " Mflops/sec quy đổi" << std::endl;

        std::cout << "\nTọa độ cuối cùng của Tâm Thiên hà 1 (Hạt số 0): (" 
                  << final_x[0] << ", " << final_y[0] << ", " << final_z[0] << ")" << std::endl;
        if (n > n/2) {
            std::cout << "Tọa độ cuối cùng của Tâm Thiên hà 2 (Hạt số " << n/2 << "): (" 
                      << final_x[n/2] << ", " << final_y[n/2] << ", " << final_z[n/2] << ")" << std::endl;
        }
    }

    // Kết thúc môi trường MPI, giải phóng tài nguyên hệ thống
    MPI_Finalize();
    return 0; // Trả về thành công
}
