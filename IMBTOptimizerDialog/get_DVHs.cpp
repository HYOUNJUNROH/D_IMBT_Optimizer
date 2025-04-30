#define ARMA_DONT_USE_BLAS
#define ARMA_DONT_USE_LAPACK
#include <armadillo>
#include <tuple>
#include <stdexcept>
#include <execution>
#include <algorithm>
#include <vector>

using namespace arma;
using namespace std;

// Custom matrix multiplication with parallel execution
vec custom_mat_mul(const mat &A, const vec &x)
{
    vec result(A.n_rows, fill::zeros);
    vector<size_t> indices(A.n_rows);
    iota(indices.begin(), indices.end(), 0);

    // Parallel execution of matrix multiplication
    for_each(execution::par_unseq, indices.begin(), indices.end(),
             [&](size_t i)
             {
                 double sum = 0.0;
                 for (uword j = 0; j < A.n_cols; j++)
                 {
                     sum += A(i, j) * x(j);
                 }
                 result(i) = sum;
             });

    return result;
}

// Improved histogram calculation to match MATLAB's hist behavior
vec calculate_single_dvh(const vec &dose_values, const vec &x)
{
    try
    {
        const uword num_bins = x.n_elem;
        vec counts = zeros<vec>(num_bins);

        // MATLAB-style binning
        double x_min = x.min();
        double x_max = x.max();
        double bin_width = (x_max - x_min) / (num_bins - 1);

        vector<double> dose_vec(dose_values.begin(), dose_values.end());
        vector<atomic<size_t>> atomic_counts(num_bins);

        // Parallel histogram calculation with improved binning
        for_each(execution::par_unseq, dose_vec.begin(), dose_vec.end(),
                 [&](const double &val)
                 {
                     if (val >= x_min && val <= x_max)
                     {
                         uword bin = static_cast<uword>((val - x_min) / bin_width);
                         if (bin >= num_bins)
                             bin = num_bins - 1;
                         atomic_counts[bin]++;
                     }
                 });

        // Copy atomic counts to regular vector
        for (uword i = 0; i < num_bins; ++i)
        {
            counts(i) = atomic_counts[i];
        }

        // Manual cumulative sum calculation
        vec dvh(num_bins);
        double sum = 0;
        for (int i = num_bins - 1; i >= 0; --i)
        {
            sum += counts(i);
            dvh(i) = sum;
        }

        // Normalize
        double max_val = dvh.max();
        if (max_val > 0)
        {
            dvh /= max_val;
        }

        return dvh;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(string("Error in DVH calculation: ") + e.what());
    }
}

// Function to process multiple OARs in parallel
tuple<vec, vec, vec, vec> process_oars_parallel(
    const mat &OAR1, const mat &OAR2, const mat &OAR3, const mat &OAR4,
    const vec &x_beam, const vec &x)
{

    vector<vec> results(4);
    vector<size_t> indices{0, 1, 2, 3};

    for_each(execution::par_unseq, indices.begin(), indices.end(),
             [&](size_t i)
             {
                 const mat &current_oar = (i == 0) ? OAR1 : (i == 1) ? OAR2
                                                        : (i == 2)   ? OAR3
                                                                     : OAR4;
                 vec oar_dummy = custom_mat_mul(current_oar, x_beam);
                 results[i] = calculate_single_dvh(oar_dummy, x);
             });

    return make_tuple(results[0], results[1], results[2], results[3]);
}

// Main get_DVHs function
tuple<double, vec, vec, vec, vec, vec>
get_DVHs(double prescription_dose,
         double normalization,
         const mat &PTV,
         const mat &OAR1,
         const mat &OAR2,
         const mat &OAR3,
         const mat &OAR4,
         const vec &x_beam,
         const vec &x,
         bool show_plot = false)
{
    try
    {
        // Enhanced input validation
        if (PTV.n_cols != x_beam.n_elem)
        {
            throw std::invalid_argument("PTV matrix columns must match x_beam length");
        }
        if (OAR1.n_cols != x_beam.n_elem || OAR2.n_cols != x_beam.n_elem ||
            OAR3.n_cols != x_beam.n_elem || OAR4.n_cols != x_beam.n_elem)
        {
            throw std::invalid_argument("OAR matrix columns must match x_beam length");
        }
        if (x.is_empty())
        {
            throw std::invalid_argument("x vector cannot be empty");
        }
        if (prescription_dose <= 0)
        {
            throw std::invalid_argument("Prescription dose must be positive");
        }
        if (normalization <= 0 || normalization > 100)
        {
            throw std::invalid_argument("Normalization must be between 0 and 100");
        }

        // Calculate PTV dose distribution
        vec PTV_dummy = custom_mat_mul(PTV, x_beam);
        cout << "PTV_dummy size: " << size(PTV_dummy) << endl;

        vec DVH_t = calculate_single_dvh(PTV_dummy, x);

        // Normalization calculation (matching MATLAB output)
        double val = normalization / 100.0;
        vec diff = abs(DVH_t - val);
        uword ind_norm = diff.index_min();
        double factor = prescription_dose / x(ind_norm);
        cout << "x(ind_norm): " << x(ind_norm) << endl;
        cout << "factor: " << factor << endl;

        // Process all OARs in parallel
        auto [DVH_OAR1, DVH_OAR2, DVH_OAR3, DVH_OAR4] =
            process_oars_parallel(OAR1, OAR2, OAR3, OAR4, x_beam, x);

        if (show_plot)
        {
            cout << "Plot functionality not implemented in C++ version\n";
        }

        return make_tuple(factor, DVH_t, DVH_OAR1, DVH_OAR2, DVH_OAR3, DVH_OAR4);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(string("Error in get_DVHs: ") + e.what());
    }
}