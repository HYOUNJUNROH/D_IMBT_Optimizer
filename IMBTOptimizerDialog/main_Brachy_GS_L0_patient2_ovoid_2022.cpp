#define ARMA_DONT_USE_BLAS
#define ARMA_DONT_USE_LAPACK
#include <armadillo>
#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>
#include <cmath>
#include <stdexcept>

using namespace arma;

// Helper function to find active angles
std::pair<int, uvec> find_active_angles(const vec &x, int NumOfAngles)
{
    int num_angles = 0;
    int seg = std::floor(x.n_elem / NumOfAngles);
    uvec active_angles = zeros<uvec>(NumOfAngles);

    for (int i = 0; i < NumOfAngles; ++i)
    {
        vec tempx = x.subvec(i * seg, (i + 1) * seg - 1);
        if (any(tempx != 0))
        {
            num_angles++;
            active_angles(i) = 1;
        }
    }
    return {num_angles, active_angles};
}

// Structure normalization function
std::pair<mat, vec> struct_normalize(const mat &input, const vec &dose)
{
    if (input.is_empty() || dose.is_empty())
    {
        throw std::runtime_error("Empty input in struct_normalize");
    }
    double max_val = input.max();
    if (std::abs(max_val) < 1e-10)
    {
        return {input, dose}; // Avoid division by zero
    }
    return {input / max_val, dose / dose.max()};
}

// Group sparsity optimization with reweighting
vec optimize_group_sparsity(const mat &A_PTV, const mat &A_OAR,
                            const vec &d_PTV, const vec &d_OAR,
                            double lambda, double c, int MaxIter)
{
    mat A = join_cols(A_PTV, A_OAR);
    vec d = join_cols(d_PTV, d_OAR);

    int n = A.n_cols;
    vec x = zeros<vec>(n);
    mat AtA = A.t() * A;
    vec Aty = A.t() * d;

    double t = 1.0 / norm(AtA, 2);
    double prev_obj = datum::inf;

    for (int iter = 0; iter < MaxIter; ++iter)
    {
        // Gradient step
        vec grad = AtA * x - Aty;
        vec x_new = x - t * grad;

        // Proximal operator for group sparsity
        x_new.transform([lambda, t](double val)
                        {
            double threshold = lambda * t;
            return std::abs(val) > threshold ? 
                   std::copysign(std::abs(val) - threshold, val) : 0.0; });

        // Check convergence
        double obj = 0.5 * norm(A * x_new - d, 2) + lambda * norm(x_new, 1);
        if (std::abs(obj - prev_obj) < 1e-6 * std::abs(prev_obj))
        {
            break;
        }
        prev_obj = obj;
        x = x_new;
    }
    return x;
}

std::tuple<cube, vec> main_Brachy_GS_L0_patient2_ovoid_2022_matlab_c__(
    double PrescriptionDose, double Normalization, int AngleResolution,
    mat PTV, mat bladder, mat rectum, mat bowel, mat sigmoid, mat body,
    uvec ind_bodyd)
{

    try
    {
        // Convert prescription dose
        PrescriptionDose /= 100.0;

        // Validate inputs
        if (PTV.is_empty() || bladder.is_empty() || rectum.is_empty() ||
            bowel.is_empty() || sigmoid.is_empty() || body.is_empty())
        {
            throw std::runtime_error("Empty input matrices");
        }

        // Scaling factor
        const double mult = 2.5e7;

        // Replace NaNs and scale matrices
        std::vector<mat *> matrices = {&PTV, &bladder, &rectum, &bowel, &sigmoid, &body};
        for (auto *matrix : matrices)
        {
            matrix->replace(datum::nan, 0);
            *matrix *= mult;
        }

        // Create target dose vector
        vec d_PTV = PrescriptionDose * ones<vec>(PTV.n_rows);

        // Normalize structures
        auto [A_PTV, d_PTV_norm] = struct_normalize(PTV, d_PTV);

        // Set optimization parameters
        vec weights = {60.0, 50.0, 20.0, 20.0};
        double sqrt_coeff = static_cast<double>(A_PTV.n_rows) /
                            (std::sqrt(weights.max()) * (bladder.n_rows + rectum.n_rows +
                                                         bowel.n_rows + sigmoid.n_rows));

        // Construct OAR matrix
        mat A_OAR = sqrt_coeff * join_cols(
                                     weights(0) * bladder,
                                     weights(1) * rectum,
                                     weights(2) * bowel,
                                     weights(3) * sigmoid);
        vec d_OAR = zeros<vec>(A_OAR.n_rows);

        // Perform optimization
        vec x = optimize_group_sparsity(A_PTV, A_OAR, d_PTV_norm, d_OAR,
                                        100.0, 10.0, 10000);

        // Calculate final dose distribution
        cube final_dose_512(512, 512, 40, fill::zeros);
        vec dose_values = body * x * 100.0; // Convert to cGy

        // Validate indices before assignment
        if (ind_bodyd.max() >= final_dose_512.n_elem ||
            dose_values.n_elem != ind_bodyd.n_elem)
        {
            throw std::runtime_error("Invalid indices or dimension mismatch");
        }

        final_dose_512.elem(ind_bodyd) = dose_values;

        return {final_dose_512, x};
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in optimization: " << e.what() << std::endl;
        throw;
    }
}