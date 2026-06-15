#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <mpi.h>

#define G 1.0
#define EPSILON 0.001

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 4 && rank == 0) {
        printf("Usage: mpirun -np <procs> %s <input_file_or_N> <steps> <dt>\n", argv[0]);
        printf("Example 1 (read file): mpirun -np 4 %s ../Data/galaxy_collision_10k.txt 100 0.01\n", argv[0]);
        printf("Example 2 (auto-generate): mpirun -np 4 %s 10000 100 0.01\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int n = 0;
    int steps = 0;
    double dt = 0.0;
    
    if (rank == 0) {
        steps = atoi(argv[2]);
        dt = atof(argv[3]);
    }

    // Broadcast basic simulation parameters
    MPI_Bcast(&steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&dt, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Temp arrays on master process
    double *master_x = NULL;
    double *master_y = NULL;
    double *master_z = NULL;
    double *master_vx = NULL;
    double *master_vy = NULL;
    double *master_vz = NULL;
    double *master_mass = NULL;

    double start_init = 0.0;
    if (rank == 0) {
        start_init = get_time();
        char *endptr;
        int parsed_n = strtol(argv[1], &endptr, 10);
        
        if (*endptr == '\0') {
            // Auto generate
            n = parsed_n;
            master_x = (double *)malloc(n * sizeof(double));
            master_y = (double *)malloc(n * sizeof(double));
            master_z = (double *)malloc(n * sizeof(double));
            master_vx = (double *)malloc(n * sizeof(double));
            master_vy = (double *)malloc(n * sizeof(double));
            master_vz = (double *)malloc(n * sizeof(double));
            master_mass = (double *)malloc(n * sizeof(double));
            
            // Keplerian disk setup
            int half_n = n / 2;
            for (int i = 0; i < n; i++) {
                double theta = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
                double r = 5.0 + ((double)rand() / RAND_MAX) * 25.0;
                
                if (i < half_n) {
                    master_x[i] = -50.0 + r * cos(theta);
                    master_y[i] = r * sin(theta);
                    master_z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
                    
                    double v_orbit = sqrt(G * 22000.0 / r);
                    master_vx[i] = 3.2 - v_orbit * sin(theta);
                    master_vy[i] = 0.6 + v_orbit * cos(theta);
                    master_vz[i] = 0.0;
                    master_mass[i] = (i == 0) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9;
                } else {
                    master_x[i] = 50.0 + r * cos(theta);
                    master_y[i] = r * sin(theta);
                    master_z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
                    
                    double v_orbit = sqrt(G * 22000.0 / r);
                    master_vx[i] = -3.2 + v_orbit * sin(theta);
                    master_vy[i] = -0.6 - v_orbit * cos(theta);
                    master_vz[i] = 0.0;
                    master_mass[i] = (i == half_n) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9;
                }
            }
        } else {
            // Read from file
            const char *filename = argv[1];
            FILE *fp = fopen(filename, "r");
            if (!fp) {
                fprintf(stderr, "Error: Cannot open input file %s\n", filename);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            
            // Count lines
            char ch;
            while(!feof(fp)) {
                ch = fgetc(fp);
                if(ch == '\n') {
                    n++;
                }
            }
            rewind(fp);
            
            master_x = (double *)malloc(n * sizeof(double));
            master_y = (double *)malloc(n * sizeof(double));
            master_z = (double *)malloc(n * sizeof(double));
            master_vx = (double *)malloc(n * sizeof(double));
            master_vy = (double *)malloc(n * sizeof(double));
            master_vz = (double *)malloc(n * sizeof(double));
            master_mass = (double *)malloc(n * sizeof(double));
            
            for (int i = 0; i < n; i++) {
                if (fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf",
                           &master_x[i], &master_y[i], &master_z[i],
                           &master_vx[i], &master_vy[i], &master_vz[i],
                           &master_mass[i]) != 7) {
                    fprintf(stderr, "Error: Invalid data format at line %d\n", i + 1);
                    fclose(fp);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
            fclose(fp);
        }
    }

    // Broadcast particle count N
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculate scattering/gathering partitions
    int *sendcounts = (int *)malloc(size * sizeof(int));
    int *displs = (int *)malloc(size * sizeof(int));
    
    int remainder = n % size;
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sendcounts[i] = n / size + (i < remainder ? 1 : 0);
        displs[i] = sum;
        sum += sendcounts[i];
    }
    
    int n_local = sendcounts[rank];
    int start_idx = displs[rank];

    // Local system allocation
    double *local_x = (double *)malloc(n_local * sizeof(double));
    double *local_y = (double *)malloc(n_local * sizeof(double));
    double *local_z = (double *)malloc(n_local * sizeof(double));
    double *local_vx = (double *)malloc(n_local * sizeof(double));
    double *local_vy = (double *)malloc(n_local * sizeof(double));
    double *local_vz = (double *)malloc(n_local * sizeof(double));
    double *local_mass = (double *)malloc(n_local * sizeof(double));

    // Scatter initial configurations from master
    MPI_Scatterv(master_x, sendcounts, displs, MPI_DOUBLE, local_x, n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_y, sendcounts, displs, MPI_DOUBLE, local_y, n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_z, sendcounts, displs, MPI_DOUBLE, local_z, n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_vx, sendcounts, displs, MPI_DOUBLE, local_vx, n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_vy, sendcounts, displs, MPI_DOUBLE, local_vy, n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_vz, sendcounts, displs, MPI_DOUBLE, local_vz, n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatterv(master_mass, sendcounts, displs, MPI_DOUBLE, local_mass, n_local, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Global arrays to be synchronized in each step
    double *global_x = (double *)malloc(n * sizeof(double));
    double *global_y = (double *)malloc(n * sizeof(double));
    double *global_z = (double *)malloc(n * sizeof(double));
    double *global_mass = (double *)malloc(n * sizeof(double));

    // Master broadcasts all masses once since they do not change
    if (rank == 0) {
        memcpy(global_mass, master_mass, n * sizeof(double));
    }
    MPI_Bcast(global_mass, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double end_init = get_time();
        printf("Initialization and Scattering time: %.4f seconds.\n", end_init - start_init);
        printf("Starting simulation: %d steps, dt = %.4f on %d processes...\n", steps, dt, size);
    }

    // Allocate local force/acceleration buffers
    double *ax = (double *)malloc(n_local * sizeof(double));
    double *ay = (double *)malloc(n_local * sizeof(double));
    double *az = (double *)malloc(n_local * sizeof(double));

    MPI_Barrier(MPI_COMM_WORLD);
    double start_sim = get_time();

    for (int step = 0; step < steps; step++) {
        // 1. Gather all coordinates to every process
        MPI_Allgatherv(local_x, n_local, MPI_DOUBLE, global_x, sendcounts, displs, MPI_DOUBLE, MPI_COMM_WORLD);
        MPI_Allgatherv(local_y, n_local, MPI_DOUBLE, global_y, sendcounts, displs, MPI_DOUBLE, MPI_COMM_WORLD);
        MPI_Allgatherv(local_z, n_local, MPI_DOUBLE, global_z, sendcounts, displs, MPI_DOUBLE, MPI_COMM_WORLD);

        // 2. Compute forces for local particles
        for (int i = 0; i < n_local; i++) {
            double acc_x = 0.0;
            double acc_y = 0.0;
            double acc_z = 0.0;
            
            double lx = local_x[i];
            double ly = local_y[i];
            double lz = local_z[i];
            
            int global_i = start_idx + i;

            for (int j = 0; j < n; j++) {
                if (global_i == j) continue;
                
                double dx = global_x[j] - lx;
                double dy = global_y[j] - ly;
                double dz = global_z[j] - lz;
                
                double distSqr = dx*dx + dy*dy + dz*dz + EPSILON*EPSILON;
                double invDist = 1.0 / sqrt(distSqr);
                double invDistCube = invDist * invDist * invDist;
                
                double f = G * global_mass[j] * invDistCube;
                
                acc_x += dx * f;
                acc_y += dy * f;
                acc_z += dz * f;
            }
            ax[i] = acc_x;
            ay[i] = acc_y;
            az[i] = acc_z;
        }

        // 3. Update local velocities and positions (Leapfrog integration)
        for (int i = 0; i < n_local; i++) {
            local_vx[i] += ax[i] * dt;
            local_vy[i] += ay[i] * dt;
            local_vz[i] += az[i] * dt;
            
            local_x[i] += local_vx[i] * dt;
            local_y[i] += local_vy[i] * dt;
            local_z[i] += local_vz[i] * dt;
        }

        if (rank == 0 && ((step + 1) % 10 == 0 || step == 0)) {
            printf("Step %d/%d completed...\n", step + 1, steps);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double end_sim = get_time();

    // Gather final coordinates to print result and verify on master
    double *final_x = NULL;
    double *final_y = NULL;
    double *final_z = NULL;
    if (rank == 0) {
        final_x = (double *)malloc(n * sizeof(double));
        final_y = (double *)malloc(n * sizeof(double));
        final_z = (double *)malloc(n * sizeof(double));
    }

    MPI_Gatherv(local_x, n_local, MPI_DOUBLE, final_x, sendcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gatherv(local_y, n_local, MPI_DOUBLE, final_y, sendcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gatherv(local_z, n_local, MPI_DOUBLE, final_z, sendcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double elapsed_sim = end_sim - start_sim;
        printf("\nSimulation finished.\n");
        printf("Total simulation time: %.4f seconds.\n", elapsed_sim);
        printf("Performance: %.2f Mflops/sec equivalent\n", (double)steps * n * n * 20.0 / (elapsed_sim * 1e6));

        printf("\nFinal position of Galaxy 1 Center (Particle 0): (%.4f, %.4f, %.4f)\n", final_x[0], final_y[0], final_z[0]);
        if (n > n/2) {
            printf("Final position of Galaxy 2 Center (Particle %d): (%.4f, %.4f, %.4f)\n", n/2, final_x[n/2], final_y[n/2], final_z[n/2]);
        }
        
        free(final_x);
        free(final_y);
        free(final_z);
    }

    // Free buffers
    free(local_x); free(local_y); free(local_z);
    free(local_vx); free(local_vy); free(local_vz);
    free(local_mass);
    free(global_x); free(global_y); free(global_z); free(global_mass);
    free(sendcounts); free(displs);
    free(ax); free(ay); free(az);
    if (rank == 0) {
        free(master_x); free(master_y); free(master_z);
        free(master_vx); free(master_vy); free(master_vz);
        free(master_mass);
    }

    MPI_Finalize();
    return 0;
}
