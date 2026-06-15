#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define G 1.0
#define EPSILON 0.001

// Structure of Arrays (SoA) for optimization
typedef struct {
    double *x, *y, *z;
    double *vx, *vy, *vz;
    double *mass;
    int n;
} ParticleSystem;

void allocate_system(ParticleSystem *sys, int n) {
    sys->n = n;
    sys->x = (double *)malloc(n * sizeof(double));
    sys->y = (double *)malloc(n * sizeof(double));
    sys->z = (double *)malloc(n * sizeof(double));
    sys->vx = (double *)malloc(n * sizeof(double));
    sys->vy = (double *)malloc(n * sizeof(double));
    sys->vz = (double *)malloc(n * sizeof(double));
    sys->mass = (double *)malloc(n * sizeof(double));
}

void free_system(ParticleSystem *sys) {
    free(sys->x);
    free(sys->y);
    free(sys->z);
    free(sys->vx);
    free(sys->vy);
    free(sys->vz);
    free(sys->mass);
}

// Read initial conditions from file
int read_input_file(const char *filename, ParticleSystem *sys) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open input file %s\n", filename);
        return 0;
    }

    // Count lines to find N
    int n = 0;
    char ch;
    while(!feof(fp)) {
        ch = fgetc(fp);
        if(ch == '\n') {
            n++;
        }
    }
    rewind(fp);

    allocate_system(sys, n);

    for (int i = 0; i < n; i++) {
        if (fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf",
                   &sys->x[i], &sys->y[i], &sys->z[i],
                   &sys->vx[i], &sys->vy[i], &sys->vz[i],
                   &sys->mass[i]) != 7) {
            fprintf(stderr, "Error: Invalid data format at line %d\n", i + 1);
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return n;
}

// Generate random initial conditions (Keplerian disk demo fallback)
void init_random_system(ParticleSystem *sys, int n) {
    allocate_system(sys, n);
    int half_n = n / 2;

    for (int i = 0; i < n; i++) {
        // Simple fallback random generation
        double theta = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
        double r = 5.0 + ((double)rand() / RAND_MAX) * 25.0;
        
        if (i < half_n) {
            // Galaxy 1
            sys->x[i] = -50.0 + r * cos(theta);
            sys->y[i] = r * sin(theta);
            sys->z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            
            double v_orbit = sqrt(G * 22000.0 / r);
            sys->vx[i] = 3.2 - v_orbit * sin(theta);
            sys->vy[i] = 0.6 + v_orbit * cos(theta);
            sys->vz[i] = 0.0;
            sys->mass[i] = (i == 0) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9;
        } else {
            // Galaxy 2
            sys->x[i] = 50.0 + r * cos(theta);
            sys->y[i] = r * sin(theta);
            sys->z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            
            double v_orbit = sqrt(G * 22000.0 / r);
            sys->vx[i] = -3.2 + v_orbit * sin(theta);
            sys->vy[i] = -0.6 - v_orbit * cos(theta);
            sys->vz[i] = 0.0;
            sys->mass[i] = (i == half_n) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9;
        }
    }
}

// Direct O(N^2) force computation & Leapfrog integration
void update_particles(ParticleSystem *sys, double dt) {
    int n = sys->n;
    
    // Temporary arrays for accelerations
    double *ax = (double *)malloc(n * sizeof(double));
    double *ay = (double *)malloc(n * sizeof(double));
    double *az = (double *)malloc(n * sizeof(double));

    // 1. Calculate accelerations
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        double acc_x = 0.0;
        double acc_y = 0.0;
        double acc_z = 0.0;
        
        double xi = sys->x[i];
        double yi = sys->y[i];
        double zi = sys->z[i];

        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            
            double dx = sys->x[j] - xi;
            double dy = sys->y[j] - yi;
            double dz = sys->z[j] - zi;
            
            double distSqr = dx*dx + dy*dy + dz*dz + EPSILON*EPSILON;
            double invDist = 1.0 / sqrt(distSqr);
            double invDistCube = invDist * invDist * invDist;
            
            double f = G * sys->mass[j] * invDistCube;
            
            acc_x += dx * f;
            acc_y += dy * f;
            acc_z += dz * f;
        }
        ax[i] = acc_x;
        ay[i] = acc_y;
        az[i] = acc_z;
    }

    // 2. Update velocities and positions (using Leapfrog-like integration)
    for (int i = 0; i < n; i++) {
        sys->vx[i] += ax[i] * dt;
        sys->vy[i] += ay[i] * dt;
        sys->vz[i] += az[i] * dt;
        
        sys->x[i] += sys->vx[i] * dt;
        sys->y[i] += sys->vy[i] * dt;
        sys->z[i] += sys->vz[i] * dt;
    }

    free(ax);
    free(ay);
    free(az);
}

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s <input_file_or_N> <steps> <dt>\n", argv[0]);
        printf("Example 1 (read file): %s ../Data/galaxy_collision_10k.txt 100 0.01\n", argv[0]);
        printf("Example 2 (auto-generate): %s 10000 100 0.01\n", argv[0]);
        return 1;
    }

    ParticleSystem sys;
    int n = 0;
    int steps = atoi(argv[2]);
    double dt = atof(argv[3]);

    char *endptr;
    int parsed_n = strtol(argv[1], &endptr, 10);

    double start_init = get_time();
    if (*endptr == '\0') {
        // Parameter 1 is an integer (N)
        n = parsed_n;
        printf("Initializing system with %d particles randomly...\n", n);
        init_random_system(&sys, n);
    } else {
        // Parameter 1 is a file path
        const char *filename = argv[1];
        printf("Reading initial conditions from file: %s...\n", filename);
        n = read_input_file(filename, &sys);
        if (n == 0) {
            return 1;
        }
        printf("Loaded %d particles successfully.\n", n);
    }
    double end_init = get_time();
    printf("Initialization time: %.4f seconds.\n", end_init - start_init);

    printf("Starting simulation: %d steps, dt = %.4f...\n", steps, dt);
    double start_sim = get_time();
    for (int step = 0; step < steps; step++) {
        update_particles(&sys, dt);
        if ((step + 1) % 10 == 0 || step == 0) {
            printf("Step %d/%d completed...\n", step + 1, steps);
        }
    }
    double end_sim = get_time();
    
    double elapsed_sim = end_sim - start_sim;
    printf("\nSimulation finished.\n");
    printf("Total simulation time: %.4f seconds.\n", elapsed_sim);
    printf("Performance: %.2f Mflops/sec equivalent\n", (double)steps * n * n * 20.0 / (elapsed_sim * 1e6));

    // Print final state of galaxy centers to verify
    printf("\nFinal position of Galaxy 1 Center (Particle 0): (%.4f, %.4f, %.4f)\n", sys.x[0], sys.y[0], sys.z[0]);
    if (n > n/2) {
        printf("Final position of Galaxy 2 Center (Particle %d): (%.4f, %.4f, %.4f)\n", n/2, sys.x[n/2], sys.y[n/2], sys.z[n/2]);
    }

    free_system(&sys);
    return 0;
}
