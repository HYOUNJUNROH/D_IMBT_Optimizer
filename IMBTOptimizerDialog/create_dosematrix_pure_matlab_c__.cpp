#pragma once

#define _USE_MATH_DEFINES
#define ARMA_DONT_USE_BLAS
#define ARMA_DONT_USE_LAPACK

#include <armadillo>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <vector>
#include <cmath>
#include <algorithm>
#include <execution>
#include <chrono>
#include <unordered_map>
#include <optional>

using namespace arma;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Data structures
struct DoseMatrixResult
{
    mat PTV;
    mat bladder;
    mat rectum;
    mat bowel;
    mat sigmoid;
    mat body;
    uvec ind_bodyd;
};

// Helper functions
struct Point3D
{
    double x, y, z;
};

// Parallel execution policy
inline constexpr auto parallel_policy = std::execution::par;

// Improved point-in-polygon test using std::views (C++17)
inline bool point_in_polygon(double x, double y, const vec &poly_x, const vec &poly_y)
{
    bool inside = false;
    const size_t n = poly_x.n_elem;

    for (size_t i = 0, j = n - 1; i < n; j = i++)
    {
        if (((poly_y(i) <= y && y < poly_y(j)) ||
             (poly_y(j) <= y && y < poly_y(i))) &&
            (x < (poly_x(j) - poly_x(i)) * (y - poly_y(i)) /
                         (poly_y(j) - poly_y(i)) +
                     poly_x(i)))
        {
            inside = !inside;
        }
    }
    return inside;
}

// Optimized interpolation using std::transform with parallel execution
template <typename T>
inline cube parallel_interp3(const cube &V, const cube &X, const cube &Y, const cube &Z)
{
    cube result(X.n_rows, X.n_cols, X.n_slices, fill::zeros);

    std::vector<Point3D> points;
    points.reserve(X.n_elem);

    for (uword k = 0; k < X.n_slices; ++k)
    {
        for (uword i = 0; i < X.n_rows; ++i)
        {
            for (uword j = 0; j < X.n_cols; ++j)
            {
                points.push_back({X(i, j, k), Y(i, j, k), Z(i, j, k)});
            }
        }
    }

    std::vector<double> interpolated_values(points.size());
    std::transform(parallel_policy, points.begin(), points.end(),
                   interpolated_values.begin(),
                   [&V](const Point3D &p)
                   {
                       if (p.x < 0 || p.y < 0 || p.z < 0 ||
                           p.x >= V.n_rows - 1 || p.y >= V.n_cols - 1 || p.z >= V.n_slices - 1)
                       {
                           return 0.0;
                       }

                       uword x0 = floor(p.x);
                       uword y0 = floor(p.y);
                       uword z0 = floor(p.z);

                       double wx = p.x - x0;
                       double wy = p.y - y0;
                       double wz = p.z - z0;

                       return V(x0, y0, z0) * (1 - wx) * (1 - wy) * (1 - wz) +
                              V(x0 + 1, y0, z0) * wx * (1 - wy) * (1 - wz) +
                              V(x0, y0 + 1, z0) * (1 - wx) * wy * (1 - wz) +
                              V(x0 + 1, y0 + 1, z0) * wx * wy * (1 - wz) +
                              V(x0, y0, z0 + 1) * (1 - wx) * (1 - wy) * wz +
                              V(x0 + 1, y0, z0 + 1) * wx * (1 - wy) * wz +
                              V(x0, y0 + 1, z0 + 1) * (1 - wx) * wy * wz +
                              V(x0 + 1, y0 + 1, z0 + 1) * wx * wy * wz;
                   });

    // Copy back to result cube
    size_t idx = 0;
    for (uword k = 0; k < result.n_slices; ++k)
    {
        for (uword i = 0; i < result.n_rows; ++i)
        {
            for (uword j = 0; j < result.n_cols; ++j)
            {
                result(i, j, k) = interpolated_values[idx++];
            }
        }
    }

    return result;
}

// Improved downsample function with parallel processing
inline cube parallel_downsample(const cube &input, int factor)
{
    const uword new_rows = input.n_rows / factor;
    const uword new_cols = input.n_cols / factor;
    const uword new_slices = input.n_slices;

    cube result(new_rows, new_cols, new_slices);

    std::vector<std::tuple<uword, uword, uword>> indices;
    indices.reserve(new_rows * new_cols * new_slices);

    for (uword k = 0; k < new_slices; ++k)
    {
        for (uword i = 0; i < new_rows; ++i)
        {
            for (uword j = 0; j < new_cols; ++j)
            {
                indices.emplace_back(i, j, k);
            }
        }
    }

    std::for_each(parallel_policy, indices.begin(), indices.end(),
                  [&](const auto &idx)
                  {
                      const auto &[i, j, k] = idx;
                      double sum = 0;
                      for (int di = 0; di < factor; ++di)
                      {
                          for (int dj = 0; dj < factor; ++dj)
                          {
                              sum += input(i * factor + di, j * factor + dj, k);
                          }
                      }
                      result(i, j, k) = sum / (factor * factor);
                  });

    return result;
}

// Data structures
struct CTData
{
    vec ImagePositionPatient;
    vec PixelSpacing;
    double SliceThickness;
    vec ImageOrientationPatient;
    cube C;
    vec xPosition;
    vec yPosition;
    vec zPosition;
};

struct StructData
{
    std::vector<std::string> structname;
    std::vector<std::vector<vec>> x;
    std::vector<std::vector<vec>> y;
    std::vector<std::vector<vec>> z;
};

struct CatheterData
{
    mat ch1;
    mat ch2;
    mat ch3;
};

struct MCData
{
    cube A;
    double sum_activity;
};

struct RotationMatrix
{
    mat rot_x;
    mat rot_y;
    mat rot_z;
};

// Helper functions
inline void create_meshgrid(const vec &x, const vec &y, mat &X, mat &Y)
{
    X = repmat(x.t(), y.n_elem, 1);
    Y = repmat(y, 1, x.n_elem);
}

// Optimized interp3 function using vectorization
inline cube interp3(const cube &V, const cube &X, const cube &Y, const cube &Z)
{
    cube result(X.n_rows, X.n_cols, X.n_slices, fill::zeros);

    for (uword k = 0; k < X.n_slices; ++k)
    {
        for (uword i = 0; i < X.n_rows; ++i)
        {
            for (uword j = 0; j < X.n_cols; ++j)
            {
                if (X(i, j, k) < 0 || Y(i, j, k) < 0 || Z(i, j, k) < 0 ||
                    X(i, j, k) >= V.n_rows - 1 || Y(i, j, k) >= V.n_cols - 1 || Z(i, j, k) >= V.n_slices - 1)
                {
                    continue;
                }

                uword x0 = floor(X(i, j, k));
                uword x1 = x0 + 1;
                uword y0 = floor(Y(i, j, k));
                uword y1 = y0 + 1;
                uword z0 = floor(Z(i, j, k));
                uword z1 = z0 + 1;

                double wx = X(i, j, k) - x0;
                double wy = Y(i, j, k) - y0;
                double wz = Z(i, j, k) - z0;

                result(i, j, k) =
                    V(x0, y0, z0) * (1 - wx) * (1 - wy) * (1 - wz) +
                    V(x1, y0, z0) * wx * (1 - wy) * (1 - wz) +
                    V(x0, y1, z0) * (1 - wx) * wy * (1 - wz) +
                    V(x1, y1, z0) * wx * wy * (1 - wz) +
                    V(x0, y0, z1) * (1 - wx) * (1 - wy) * wz +
                    V(x1, y0, z1) * wx * (1 - wy) * wz +
                    V(x0, y1, z1) * (1 - wx) * wy * wz +
                    V(x1, y1, z1) * wx * wy * wz;
            }
        }
    }
    return result;
}

inline cube load_ovoid_data(const std::string &filename)
{
    // Implementation based on MATLAB .mat file loading
    cube result(200, 200, 200, fill::zeros);

    // Load MC data
    if (filename == "systemA_2nd")
    {
        // Load tandem MC data
        cube data(200, 200, 200, fill::zeros);
        data.load("systemA_2nd.mat"); // Load .mat file containing MC data
        result = data;
    }
    else if (filename.find("systemA_ovoid") != std::string::npos)
    {
        // Load ovoid MC data
        if (filename.find("_left") != std::string::npos)
        {
            // Left ovoid data
            if (filename == "systemA_ovoid1_left")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid1_left.mat");
                result = data;
            }
            else if (filename == "systemA_ovoid2_left")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid2_left.mat");
                result = data;
            }
            else if (filename == "systemA_ovoid3_left")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid3_left.mat");
                result = data;
            }
            else if (filename == "systemA_ovoid4_left")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid4_left.mat");
                result = data;
            }
            else if (filename == "systemA_ovoid5_left")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid5_left.mat");
                result = data;
            }
        }
        else
        {
            // Right ovoid data - A2, B2, C2, D2, E2 matrices
            if (filename == "systemA_ovoid1_right")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid1_right.mat");
                result = data;
            }
            else if (filename == "systemA_ovoid2_right")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid2_right.mat");
                result = data;
            }
            else if (filename == "systemA_ovoid3_right")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid3_right.mat");
                result = data;
            }
            else if (filename == "systemA_ovoid4_right")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid4_right.mat");
                result = data;
            }
            else if (filename == "systemA_ovoid5_right")
            {
                cube data(200, 200, 200, fill::zeros);
                data.load("systemA_ovoid5_right.mat");
                result = data;
            }
        }
    }

    // Remove negative values
    result.elem(find(result < 0)).zeros();

    return result;
}

inline RotationMatrix create_rotation_matrices(double theta_x, double theta_y, double theta_z)
{
    RotationMatrix rot;
    double rx = theta_x * M_PI / 180.0;
    double ry = theta_y * M_PI / 180.0;
    double rz = theta_z * M_PI / 180.0;

    rot.rot_x = {{1, 0, 0},
                 {0, cos(rx), -sin(rx)},
                 {0, sin(rx), cos(rx)}};

    rot.rot_y = {{cos(ry), 0, sin(ry)},
                 {0, 1, 0},
                 {-sin(ry), 0, cos(ry)}};

    rot.rot_z = {{cos(rz), -sin(rz), 0},
                 {sin(rz), cos(rz), 0},
                 {0, 0, 1}};

    return rot;
}

// Improved MC data loading with caching
inline MCData load_mc_data(const std::string &filename, double normalization_factor)
{
    static std::unordered_map<std::string, cube> cache;
    MCData result;

    auto it = cache.find(filename);
    if (it != cache.end())
    {
        result.A = it->second;
    }
    else
    {
        result.A = load_ovoid_data(filename);
        cache[filename] = result.A;
    }

    result.sum_activity = accu(result.A);
    result.A *= (normalization_factor / result.sum_activity);
    return result;
}

inline mat extract_structure_values(const cube &dose,
                                    const cube &PTV,
                                    const cube &bladder,
                                    const cube &rectum,
                                    const cube &bowel,
                                    const cube &sigmoid,
                                    const cube &body)
{
    mat result(6, 1);

    // Element-wise multiplication and sum
    result(0) = accu(dose % PTV);
    result(1) = accu(dose % bladder);
    result(2) = accu(dose % rectum);
    result(3) = accu(dose % bowel);
    result(4) = accu(dose % sigmoid);
    result(5) = accu(dose % body);

    return result;
}

// Forward declaration
void process_structure(const cube &mask, mat &result,
                       const MCData &tandem_mc,
                       const std::vector<MCData> &ovoid_mc_left,
                       const std::vector<MCData> &ovoid_mc_right,
                       const mat &source_pos,
                       const vec3 &pA, const vec3 &pB, const vec3 &pC);

// Forward declarations for rotation matrix functions
inline mat createRotationMatrixX(double theta);
inline mat createRotationMatrixY(double theta);
inline mat createRotationMatrixZ(double theta);

// Contour to indices conversion based on MATLAB insidepoly()
uvec convert_contour_to_indices(const vec &xc, const vec &yc, const mat &XC, const mat &YC)
{
    uvec indices;
    size_t n = xc.n_elem;

    // Check each point in XC,YC grid if inside polygon defined by xc,yc
    for (size_t i = 0; i < XC.n_rows; i++)
    {
        for (size_t j = 0; j < XC.n_cols; j++)
        {
            if (point_in_polygon(XC(i, j), YC(i, j), xc, yc))
            {
                indices = join_cols(indices, uvec({i * XC.n_cols + j}));
            }
        }
    }

    return indices;
}

// Main function
DoseMatrixResult create_dose_matrix(const std::string &PatientID,
                                    const CTData &ctData,
                                    const StructData &structData,
                                    const CatheterData &catheter,
                                    int angleResolution)
{
    // Initialize timing and result
    auto start_time = std::chrono::high_resolution_clock::now();
    DoseMatrixResult result;

    // Get dimensions
    const uword nrows = ctData.C.n_rows;
    const uword ncols = ctData.C.n_cols;
    const uword nslices = ctData.C.n_slices;

    // Create binary masks - Move these declarations to the top
    cube mask_PTV = zeros<cube>(nrows / 2, ncols / 2, nslices);
    cube mask_bladder = zeros<cube>(nrows / 2, ncols / 2, nslices);
    cube mask_rectum = zeros<cube>(nrows / 2, ncols / 2, nslices);
    cube mask_bowel = zeros<cube>(nrows / 2, ncols / 2, nslices);
    cube mask_sigmoid = zeros<cube>(nrows / 2, ncols / 2, nslices);
    cube mask_body = zeros<cube>(nrows / 4, ncols / 4, nslices);

    // Process MC data - Move this before it's used
    MCData tandem_mc = load_mc_data("systemA_2nd", 1.0);
    std::vector<MCData> ovoid_mc_left, ovoid_mc_right;

    // Constants
    const double zC_top = 106.0;
    const bool flag_body = true;

    // Calculate rotation angles
    double theta_x = std::acos(ctData.ImageOrientationPatient(4)) * 180.0 / M_PI;
    double theta_y = std::acos(ctData.ImageOrientationPatient(2)) * 180.0 / M_PI - 90.0;
    double theta_z = std::acos(ctData.ImageOrientationPatient(1)) * 180.0 / M_PI - 90.0;

    // Create coordinate systems
    vec xC = linspace(ctData.ImagePositionPatient(0),
                      ctData.ImagePositionPatient(0) + ctData.PixelSpacing(0) * 511,
                      512);
    vec yC = linspace(ctData.ImagePositionPatient(1),
                      ctData.ImagePositionPatient(1) + ctData.PixelSpacing(1) * 511,
                      512);
    vec zC = linspace(zC_top, zC_top - mean(diff(ctData.zPosition)) * (nslices - 1), nslices);

    // Create meshgrids
    mat XCT, YCT;
    create_meshgrid(xC, yC, XCT, YCT);

    // Create rotation matrices
    mat rot_x = createRotationMatrixX(theta_x);
    mat rot_y = createRotationMatrixY(theta_y);
    mat rot_z = createRotationMatrixZ(theta_z);

    // Define transformation constants
    const double x_trans = -1.0;
    const double y_trans = 1.0;
    const double z_trans = 0.0;
    vec hist_zloc_body;

    // Process BODY contours first
    for (size_t i = 0; i < structData.structname.size(); i++)
    {
        if (structData.structname[i] == "BODY")
        {
            for (size_t j = 0; j < structData.x[i].size(); j++)
            {
                mat locs = join_rows(structData.x[i][j],
                                     join_rows(structData.y[i][j],
                                               structData.z[i][j]));
                mat locs_new = locs * rot_x * rot_y * rot_z;
                vec tempz = locs_new.col(2);

                double zloc = tempz(0);
                zloc = round(zloc * 1000.0) / 1000.0;
                hist_zloc_body = join_vert(hist_zloc_body, vec({zloc}));
            }
            break;
        }
    }

    // Sort and adjust hist_zloc_body
    if (!hist_zloc_body.empty())
    {
        hist_zloc_body = sort(hist_zloc_body + z_trans, "descend");
    }

    // Process structures
    for (size_t i = 0; i < structData.structname.size(); ++i)
    {
        if (structData.structname[i] == "BODY" ||
            structData.structname[i] == "HR-CTV" ||
            structData.structname[i] == "BLADDER" ||
            structData.structname[i] == "RECTUM" ||
            structData.structname[i] == "BOWEL" ||
            structData.structname[i] == "SIGMOID")
        {
            for (size_t j = 0; j < structData.x[i].size(); ++j)
            {
                mat locs = join_rows(structData.x[i][j],
                                     join_rows(structData.y[i][j],
                                               structData.z[i][j]));

                mat locs_new = locs * rot_x * rot_y * rot_z;
                vec tempx = locs_new.col(0) + x_trans;
                vec tempy = locs_new.col(1) + y_trans;
                vec tempz = locs_new.col(2) + z_trans;

                double zloc = tempz(0);
                zloc = round(zloc * 1000.0) / 1000.0;

                // Find matching slice
                uvec citemp;
                if (!hist_zloc_body.empty())
                {
                    citemp = find(abs(hist_zloc_body - zloc) <= 1.0);
                }

                if (!citemp.empty())
                {
                    uvec local_indices = convert_contour_to_indices(tempx, tempy, XCT, YCT);
                    if (structData.structname[i] == "HR-CTV")
                    {
                        for (uword idx : local_indices)
                        {
                            uword slice = citemp(0);
                            uword y = idx / nrows;
                            uword x = idx % nrows;
                            mask_PTV(x / 2, y / 2, slice - 1) = 1;
                        }
                    }
                    else if (structData.structname[i] == "BLADDER")
                    {
                        for (uword idx : local_indices)
                        {
                            uword slice = citemp(0);
                            uword y = idx / nrows;
                            uword x = idx % nrows;
                            mask_bladder(x / 2, y / 2, slice - 1) = 1;
                        }
                    }
                    else if (structData.structname[i] == "RECTUM")
                    {
                        for (uword idx : local_indices)
                        {
                            uword slice = citemp(0);
                            uword y = idx / nrows;
                            uword x = idx % nrows;
                            mask_rectum(x / 2, y / 2, slice - 1) = 1;
                        }
                    }
                    else if (structData.structname[i] == "BOWEL")
                    {
                        for (uword idx : local_indices)
                        {
                            uword slice = citemp(0);
                            uword y = idx / nrows;
                            uword x = idx % nrows;
                            mask_bowel(x / 2, y / 2, slice - 1) = 1;
                        }
                    }
                    else if (structData.structname[i] == "SIGMOID")
                    {
                        for (uword idx : local_indices)
                        {
                            uword slice = citemp(0);
                            uword y = idx / nrows;
                            uword x = idx % nrows;
                            mask_sigmoid(x / 2, y / 2, slice - 1) = 1;
                        }
                    }
                    else if (structData.structname[i] == "BODY")
                    {
                        for (uword idx : local_indices)
                        {
                            uword slice = citemp(0);
                            uword y = idx / nrows;
                            uword x = idx % nrows;
                            mask_body(x / 4, y / 4, slice - 1) = 1;
                        }
                    }
                }
            }
        }
    }

    // Create source positions
    double resolution = 2.5; // mm
    vec angle_pos = linspace<vec>(-180, 170, static_cast<uword>(std::ceil(350.0 / angleResolution)) + 1);

    // Process source positions
    size_t total_positions = catheter.ch3.n_rows * angle_pos.n_elem;
    result.PTV = zeros<mat>(mask_PTV.n_elem, total_positions);
    result.bladder = zeros<mat>(mask_bladder.n_elem, total_positions);
    result.rectum = zeros<mat>(mask_rectum.n_elem, total_positions);
    result.bowel = zeros<mat>(mask_bowel.n_elem, total_positions);
    result.sigmoid = zeros<mat>(mask_sigmoid.n_elem, total_positions);
    result.body = zeros<mat>(mask_body.n_elem, total_positions);

    // Process positions
    for (size_t i = 0; i < catheter.ch3.n_rows; ++i)
    {
        for (size_t j = 0; j < angle_pos.n_elem; ++j)
        {
            // Process position
            double theta_x2, theta_y2;
            if (i == 0)
            {
                theta_x2 = (atan((catheter.ch3(i, 1) - catheter.ch3(i + 1, 1)) / (catheter.ch3(i, 2) - catheter.ch3(i + 1, 2) + 0.000001)) * 180 / M_PI);
                theta_y2 = (-atan((catheter.ch3(i, 0) - catheter.ch3(i + 1, 0)) / (catheter.ch3(i, 2) - catheter.ch3(i + 1, 2) + 0.000001)) * 180 / M_PI);
            }
            else
            {
                theta_x2 = (atan((catheter.ch3(i - 1, 1) - catheter.ch3(i, 1)) / (catheter.ch3(i - 1, 2) - catheter.ch3(i, 2) + 0.000001)) * 180 / M_PI);
                theta_y2 = (-atan((catheter.ch3(i - 1, 0) - catheter.ch3(i, 0)) / (catheter.ch3(i - 1, 2) - catheter.ch3(i, 2) + 0.000001)) * 180 / M_PI);
            }
            double theta_z2 = angle_pos(j);

            // Create rotation matrices and transform MC data
            auto rot = create_rotation_matrices(theta_x2, theta_y2, theta_z2);
            cube Arot = tandem_mc.A;
            size_t col_idx = i * angle_pos.n_elem + j;

            // Apply rotations
            mat R = rot.rot_z * rot.rot_y * rot.rot_x;
            cube Arot_new = zeros<cube>(size(Arot));

            // Create coordinate system for current position
            vec xt = linspace(catheter.ch3(i, 0) - 100, catheter.ch3(i, 0) + 100, 201);
            vec yt = linspace(catheter.ch3(i, 1) - 100, catheter.ch3(i, 1) + 100, 201);
            vec zt = linspace(catheter.ch3(i, 2) + 100, catheter.ch3(i, 2) - 100, 201);

            cube Xt, Yt, Zt;
            Xt.set_size(size(Arot));
            Yt.set_size(size(Arot));
            Zt.set_size(size(Arot));

            for (uword k = 0; k < Arot.n_slices; ++k)
            {
                for (uword i = 0; i < Arot.n_rows; ++i)
                {
                    for (uword j = 0; j < Arot.n_cols; ++j)
                    {
                        vec point = {double(i) - 100, double(j) - 100, double(k) - 100};
                        vec rotated = R * point;
                        Xt(i, j, k) = rotated(0) + catheter.ch3(i, 0);
                        Yt(i, j, k) = rotated(1) + catheter.ch3(i, 1);
                        Zt(i, j, k) = rotated(2) + catheter.ch3(i, 2);
                    }
                }
            }

            Arot_new = interp3(tandem_mc.A, Xt, Yt, Zt);

            cube Arot_newd = zeros<cube>(mask_PTV.n_rows, mask_PTV.n_cols, mask_PTV.n_slices);
            cube Arot_newd_4 = zeros<cube>(mask_body.n_rows, mask_body.n_cols, mask_body.n_slices);

            for (uword ss = 0; ss < mask_PTV.n_slices; ++ss)
            {
                for (uword row = 0; row < mask_PTV.n_rows; ++row)
                {
                    for (uword col = 0; col < mask_PTV.n_cols; ++col)
                    {
                        Arot_newd(row, col, ss) = Arot_new(row * 2, col * 2, ss);
                    }
                }
            }

            for (uword ss = 0; ss < mask_body.n_slices; ++ss)
            {
                for (uword row = 0; row < mask_body.n_rows; ++row)
                {
                    for (uword col = 0; col < mask_body.n_cols; ++col)
                    {
                        Arot_newd_4(row, col, ss) = Arot_new(row * 4, col * 4, ss);
                    }
                }
            }

            // Extract values for structure
            uvec structure_indices_PTV = find(mask_PTV > 0);
            uvec structure_indices_bladder = find(mask_bladder > 0);
            uvec structure_indices_rectum = find(mask_rectum > 0);
            uvec structure_indices_bowel = find(mask_bowel > 0);
            uvec structure_indices_sigmoid = find(mask_sigmoid > 0);
            uvec structure_indices_body = find(mask_body > 0);

            result.PTV.col(col_idx) = Arot_newd(structure_indices_PTV);
            result.bladder.col(col_idx) = Arot_newd(structure_indices_bladder);
            result.rectum.col(col_idx) = Arot_newd(structure_indices_rectum);
            result.bowel.col(col_idx) = Arot_newd(structure_indices_bowel);
            result.sigmoid.col(col_idx) = Arot_newd(structure_indices_sigmoid);
            result.body.col(col_idx) = Arot_newd_4(structure_indices_body);
        }
    }

    // Process MC data
    ovoid_mc_left.clear();
    ovoid_mc_right.clear();
    for (int i = 1; i <= 5; ++i)
    {
        ovoid_mc_left.push_back(load_mc_data("systemA_ovoid" + std::to_string(i) + "_left", tandem_mc.sum_activity));
        ovoid_mc_right.push_back(load_mc_data("systemA_ovoid" + std::to_string(i) + "_right", tandem_mc.sum_activity));
    }

    // Process each structure
    process_structure(mask_PTV, result.PTV, tandem_mc, ovoid_mc_left, ovoid_mc_right, catheter.ch3, vec3({catheter.ch1(0, 0), catheter.ch1(0, 1), catheter.ch1(0, 2)}), vec3({catheter.ch1(1, 0), catheter.ch1(1, 1), catheter.ch1(1, 2)}), vec3({catheter.ch1(2, 0), catheter.ch1(2, 1), catheter.ch1(2, 2)}));
    process_structure(mask_bladder, result.bladder, tandem_mc, ovoid_mc_left, ovoid_mc_right, catheter.ch3, vec3({catheter.ch1(0, 0), catheter.ch1(0, 1), catheter.ch1(0, 2)}), vec3({catheter.ch1(1, 0), catheter.ch1(1, 1), catheter.ch1(1, 2)}), vec3({catheter.ch1(2, 0), catheter.ch1(2, 1), catheter.ch1(2, 2)}));
    process_structure(mask_rectum, result.rectum, tandem_mc, ovoid_mc_left, ovoid_mc_right, catheter.ch3, vec3({catheter.ch1(0, 0), catheter.ch1(0, 1), catheter.ch1(0, 2)}), vec3({catheter.ch1(1, 0), catheter.ch1(1, 1), catheter.ch1(1, 2)}), vec3({catheter.ch1(2, 0), catheter.ch1(2, 1), catheter.ch1(2, 2)}));
    process_structure(mask_bowel, result.bowel, tandem_mc, ovoid_mc_left, ovoid_mc_right, catheter.ch3, vec3({catheter.ch1(0, 0), catheter.ch1(0, 1), catheter.ch1(0, 2)}), vec3({catheter.ch1(1, 0), catheter.ch1(1, 1), catheter.ch1(1, 2)}), vec3({catheter.ch1(2, 0), catheter.ch1(2, 1), catheter.ch1(2, 2)}));
    process_structure(mask_sigmoid, result.sigmoid, tandem_mc, ovoid_mc_left, ovoid_mc_right, catheter.ch3, vec3({catheter.ch1(0, 0), catheter.ch1(0, 1), catheter.ch1(0, 2)}), vec3({catheter.ch1(1, 0), catheter.ch1(1, 1), catheter.ch1(1, 2)}), vec3({catheter.ch1(2, 0), catheter.ch1(2, 1), catheter.ch1(2, 2)}));

    // Recalculate rotation angles and create matrices
    double rx = theta_x * M_PI / 180.0;
    double ry = theta_y * M_PI / 180.0;
    double rz = theta_z * M_PI / 180.0;

    // Define transformation constant
    vec hist_zloc;

    // Process BODY contours first to get hist_zloc_body
    double z_trans_temp = 0.0;
    for (size_t i = 0; i < structData.structname.size(); i++)
    {
        if (structData.structname[i] == "BODY")
        {
            for (size_t j = 0; j < structData.x[i].size(); j++)
            {
                mat locs = join_rows(structData.x[i][j],
                                     join_rows(structData.y[i][j],
                                               structData.z[i][j]));
                mat locs_new = locs * rot_x * rot_y * rot_z;
                vec tempz = locs_new.col(2);

                double zloc = tempz(0);
                zloc = round(zloc * 1000.0) / 1000.0;
                hist_zloc_body = join_vert(hist_zloc_body, vec({zloc}));
            }

            z_trans_temp = zC_top - max(hist_zloc_body);
            hist_zloc_body.transform([z_trans_temp](double val)
                                     { return val + z_trans_temp; });
            hist_zloc_body = sort(hist_zloc_body, "descend");
            break;
        }
    }

    // Process structures
    for (size_t i = 0; i < structData.structname.size(); ++i)
    {
        if (structData.structname[i] == "BODY" ||
            structData.structname[i] == "HR-CTV" ||
            structData.structname[i] == "BLADDER" ||
            structData.structname[i] == "RECTUM" ||
            structData.structname[i] == "BOWEL" ||
            structData.structname[i] == "SIGMOID")
        {
            for (size_t j = 0; j < structData.x[i].size(); ++j)
            {
                mat locs = join_rows(structData.x[i][j],
                                     join_rows(structData.y[i][j],
                                               structData.z[i][j]));

                mat locs_new = locs * rot_x * rot_y * rot_z;
                vec tempx = locs_new.col(0) + x_trans;
                vec tempy = locs_new.col(1) + y_trans;
                vec tempz = locs_new.col(2) + z_trans;

                double zloc = tempz(0);
                zloc = round(zloc * 1000.0) / 1000.0;
                uvec citemp = find(abs(hist_zloc_body - zloc) <= 1.0);

                if (!citemp.empty())
                {
                    mat XC_slice = XCT;
                    mat YC_slice = YCT;
                    uvec local_indices = convert_contour_to_indices(tempx, tempy, XC_slice, YC_slice);

                    for (uword idx : local_indices)
                    {
                        uword slice = citemp(0);
                        uword y = idx / 512;
                        uword x = idx % 512;

                        if (structData.structname[i] == "HR-CTV")
                        {
                            mask_PTV(x / 2, y / 2, slice - 1) = 1;
                        }
                        else if (structData.structname[i] == "BLADDER")
                        {
                            mask_bladder(x / 2, y / 2, slice - 1) = 1;
                        }
                        else if (structData.structname[i] == "RECTUM")
                        {
                            mask_rectum(x / 2, y / 2, slice - 1) = 1;
                        }
                        else if (structData.structname[i] == "BOWEL")
                        {
                            mask_bowel(x / 2, y / 2, slice - 1) = 1;
                        }
                        else if (structData.structname[i] == "SIGMOID")
                        {
                            mask_sigmoid(x / 2, y / 2, slice - 1) = 1;
                        }
                        else if (structData.structname[i] == "BODY")
                        {
                            mask_body(x / 4, y / 4, slice - 1) = 1;
                        }
                    }
                }
            }
        }
    }

    return result;
}

// Optimized process_structure implementation
void process_structure(const cube &mask, mat &result,
                       const MCData &tandem_mc,
                       const std::vector<MCData> &ovoid_mc_left,
                       const std::vector<MCData> &ovoid_mc_right,
                       const mat &source_pos,
                       const vec3 &pA, const vec3 &pB, const vec3 &pC)
{
    // Constants
    const double resolution = 2.5;        // mm
    const double angle_resolution = 60.0; // degrees
    const vec angle_pos = linspace<vec>(-180, 170, static_cast<uword>(std::ceil(350.0 / angle_resolution)) + 1);

    // Initialize counters
    size_t cnt = 0;

    // Process tandem positions
    for (size_t jj = 0; jj < angle_pos.n_elem; ++jj)
    {
        for (size_t ii = 0; ii < source_pos.n_rows; ++ii)
        {
            // Calculate rotation angles based on source positions
            double theta_x2, theta_y2, theta_z2;
            if (ii == 0)
            {
                theta_x2 = atan2(source_pos(ii, 1) - source_pos(ii + 1, 1),
                                 source_pos(ii, 2) - source_pos(ii + 1, 2)) *
                           180 / M_PI;
                theta_y2 = -atan2(source_pos(ii, 0) - source_pos(ii + 1, 0),
                                  source_pos(ii, 2) - source_pos(ii + 1, 2)) *
                           180 / M_PI;
            }
            else
            {
                theta_x2 = atan2(source_pos(ii - 1, 1) - source_pos(ii, 1),
                                 source_pos(ii - 1, 2) - source_pos(ii, 2)) *
                           180 / M_PI;
                theta_y2 = -atan2(source_pos(ii - 1, 0) - source_pos(ii, 0),
                                  source_pos(ii - 1, 2) - source_pos(ii, 2)) *
                           180 / M_PI;
            }
            theta_z2 = angle_pos(jj);

            // Create coordinate system for current position
            vec xt = linspace(source_pos(ii, 0) - 100, source_pos(ii, 0) + 100, 201);
            vec yt = linspace(source_pos(ii, 1) - 100, source_pos(ii, 1) + 100, 201);
            vec zt = linspace(source_pos(ii, 2) + 100, source_pos(ii, 2) - 100, 201);

            // Rotate tandem MC data
            cube Arot = tandem_mc.A;

            // Create rotation matrices
            auto rot = create_rotation_matrices(theta_x2, theta_y2, theta_z2);

            // Apply rotations
            cube Xt, Yt, Zt;
            Xt.set_size(size(Arot));
            Yt.set_size(size(Arot));
            Zt.set_size(size(Arot));

            for (uword k = 0; k < Arot.n_slices; ++k)
            {
                for (uword i = 0; i < Arot.n_rows; ++i)
                {
                    for (uword j = 0; j < Arot.n_cols; ++j)
                    {
                        vec point = {double(i) - 100, double(j) - 100, double(k) - 100};
                        vec rotated = rot.rot_z * rot.rot_y * rot.rot_x * point;
                        Xt(i, j, k) = rotated(0) + source_pos(ii, 0);
                        Yt(i, j, k) = rotated(1) + source_pos(ii, 1);
                        Zt(i, j, k) = rotated(2) + source_pos(ii, 2);
                    }
                }
            }

            // Interpolate rotated data onto patient coordinate system
            cube Arot_new = interp3(Arot, Xt, Yt, Zt);

            // Downsample for efficiency
            cube Arot_newd = parallel_downsample(Arot_new, 2);

            // Extract values for structure
            uvec structure_indices = find(mask > 0);
            result.col(cnt) = Arot_newd(structure_indices);

            cnt++;
        }
    }

    // Process ovoid positions
    for (size_t i = 0; i < ovoid_mc_left.size(); ++i)
    {
        // Process left ovoid
        {
            // Create ovoid coordinate system
            vec xt = linspace(pC(0) - 99.5, pC(0) + 99.5, 200);
            vec yt = linspace(pC(1) - 99.5, pC(1) + 99.5, 200);
            vec zt = linspace(pC(2) - 99.5, pC(2) + 99.5, 200);

            // Calculate rotation angles
            double theta_x2 = atan2(pB(1) - pA(1), pB(2) - pA(2)) * 180 / M_PI;
            double theta_y2 = -atan2(pB(0) - pA(0), pB(2) - pA(2)) * 180 / M_PI;

            // Rotate and interpolate left ovoid data
            cube Al = ovoid_mc_left[i].A;
            auto rot = create_rotation_matrices(theta_x2, theta_y2, 0);

            // Create coordinate grids for interpolation
            cube Xt, Yt, Zt;
            Xt.set_size(size(Al));
            Yt.set_size(size(Al));
            Zt.set_size(size(Al));

            // Transform coordinates
            for (uword k = 0; k < Al.n_slices; ++k)
            {
                for (uword i = 0; i < Al.n_rows; ++i)
                {
                    for (uword j = 0; j < Al.n_cols; ++j)
                    {
                        vec point = {double(i) - 100, double(j) - 100, double(k) - 100};
                        vec rotated = rot.rot_z * rot.rot_y * rot.rot_x * point;
                        Xt(i, j, k) = rotated(0) + pC(0);
                        Yt(i, j, k) = rotated(1) + pC(1);
                        Zt(i, j, k) = rotated(2) + pC(2);
                    }
                }
            }

            cube Arot1 = interp3(Al, Xt, Yt, Zt);
            cube Arot1_ds = parallel_downsample(Arot1, 2);

            result.col(cnt++) = Arot1_ds(find(mask > 0));
        }

        // Process right ovoid
        {
            // Create ovoid coordinate system (same as left)
            vec xt = linspace(pC(0) - 99.5, pC(0) + 99.5, 200);
            vec yt = linspace(pC(1) - 99.5, pC(1) + 99.5, 200);
            vec zt = linspace(pC(2) - 99.5, pC(2) + 99.5, 200);

            // Calculate rotation angles (same as left)
            double theta_x2 = atan2(pB(1) - pA(1), pB(2) - pA(2)) * 180 / M_PI;
            double theta_y2 = -atan2(pB(0) - pA(0), pB(2) - pA(2)) * 180 / M_PI;

            // Rotate and interpolate right ovoid data
            cube Ar = ovoid_mc_right[i].A;
            auto rot = create_rotation_matrices(theta_x2, theta_y2, 0);

            // Create coordinate grids
            cube Xt, Yt, Zt;
            Xt.set_size(size(Ar));
            Yt.set_size(size(Ar));
            Zt.set_size(size(Ar));

            // Transform coordinates
            for (uword k = 0; k < Ar.n_slices; ++k)
            {
                for (uword i = 0; i < Ar.n_rows; ++i)
                {
                    for (uword j = 0; j < Ar.n_cols; ++j)
                    {
                        vec point = {double(i) - 100, double(j) - 100, double(k) - 100};
                        vec rotated = rot.rot_z * rot.rot_y * rot.rot_x * point;
                        Xt(i, j, k) = rotated(0) + pC(0);
                        Yt(i, j, k) = rotated(1) + pC(1);
                        Zt(i, j, k) = rotated(2) + pC(2);
                    }
                }
            }

            cube Arot2 = interp3(Ar, Xt, Yt, Zt);
            cube Arot2_ds = parallel_downsample(Arot2, 2);

            result.col(cnt++) = Arot2_ds(find(mask > 0));
        }
    }
}

// Add helper functions to match MATLAB functionality
inline mat rigidtform3d(const vec &theta, const vec &transl)
{
    double rx = theta(0) * M_PI / 180.0;
    double ry = theta(1) * M_PI / 180.0;
    double rz = theta(2) * M_PI / 180.0;

    mat rot_x = {{1, 0, 0},
                 {0, cos(rx), -sin(rx)},
                 {0, sin(rx), cos(rx)}};

    mat rot_y = {{cos(ry), 0, sin(ry)},
                 {0, 1, 0},
                 {-sin(ry), 0, cos(ry)}};

    mat rot_z = {{cos(rz), -sin(rz), 0},
                 {sin(rz), cos(rz), 0},
                 {0, 0, 1}};

    mat R = rot_z * rot_y * rot_x;
    mat T = join_rows(R, transl);
    T = join_cols(T, mat({0, 0, 0, 1}));
    return T;
}

inline cube imwarp(const cube &A, const mat &tform, const uvec3 &outputView)
{
    cube result(outputView(0), outputView(1), outputView(2), fill::zeros);

    // Extract rotation and translation
    mat R = tform(span(0, 2), span(0, 2));
    vec t = tform(span(0, 2), 3);

    // Create coordinate grids
    for (uword k = 0; k < result.n_slices; ++k)
    {
        for (uword i = 0; i < result.n_rows; ++i)
        {
            for (uword j = 0; j < result.n_cols; ++j)
            {
                vec p = {double(i), double(j), double(k), 1.0};
                vec transformed = tform * p;

                // Interpolate value
                double x = transformed(0);
                double y = transformed(1);
                double z = transformed(2);

                if (x >= 0 && x < A.n_rows - 1 &&
                    y >= 0 && y < A.n_cols - 1 &&
                    z >= 0 && z < A.n_slices - 1)
                {

                    uword x0 = floor(x);
                    uword y0 = floor(y);
                    uword z0 = floor(z);

                    double wx = x - x0;
                    double wy = y - y0;
                    double wz = z - z0;

                    result(i, j, k) =
                        A(x0, y0, z0) * (1 - wx) * (1 - wy) * (1 - wz) +
                        A(x0 + 1, y0, z0) * wx * (1 - wy) * (1 - wz) +
                        A(x0, y0 + 1, z0) * (1 - wx) * wy * (1 - wz) +
                        A(x0 + 1, y0 + 1, z0) * wx * wy * (1 - wz) +
                        A(x0, y0, z0 + 1) * (1 - wx) * (1 - wy) * wz +
                        A(x0 + 1, y0, z0 + 1) * wx * (1 - wy) * wz +
                        A(x0, y0 + 1, z0 + 1) * (1 - wx) * wy * wz +
                        A(x0 + 1, y0 + 1, z0 + 1) * wx * wy * wz;
                }
            }
        }
    }
    return result;
}

DoseMatrixResult create_dosematrix_pure_matlab_c__(
    const std::string &PatientID,
    const json &ctData,
    const json &structData,
    const json &catheterData,
    double angleResolution)
{
    // Convert JSON to internal types
    CTData ct;
    ct.ImagePositionPatient = vec(ctData["ImagePositionPatient"].get<std::vector<double>>());
    ct.PixelSpacing = vec(ctData["PixelSpacing"].get<std::vector<double>>());
    ct.SliceThickness = ctData["SliceThickness"].get<double>();
    ct.ImageOrientationPatient = vec(ctData["ImageOrientationPatient"].get<std::vector<double>>());

    // Convert CT data cube
    auto ctCube = ctData["C"].get<std::vector<std::vector<std::vector<double>>>>();
    ct.C = cube(ctCube.size(), ctCube[0].size(), ctCube[0][0].size());
    for (size_t i = 0; i < ctCube.size(); i++)
    {
        for (size_t j = 0; j < ctCube[0].size(); j++)
        {
            for (size_t k = 0; k < ctCube[0][0].size(); k++)
            {
                ct.C(i, j, k) = ctCube[i][j][k];
            }
        }
    }

    ct.xPosition = vec(ctData["xPosition"].get<std::vector<double>>());
    ct.yPosition = vec(ctData["yPosition"].get<std::vector<double>>());
    ct.zPosition = vec(ctData["zPosition"].get<std::vector<double>>());

    // Convert structure data
    StructData sd;
    sd.structname = structData["structname"].get<std::vector<std::string>>();

    auto x_data = structData["x"].get<std::vector<std::vector<std::vector<double>>>>();
    auto y_data = structData["y"].get<std::vector<std::vector<std::vector<double>>>>();
    auto z_data = structData["z"].get<std::vector<std::vector<std::vector<double>>>>();

    for (size_t i = 0; i < x_data.size(); i++)
    {
        std::vector<vec> x_vecs, y_vecs, z_vecs;
        for (size_t j = 0; j < x_data[i].size(); j++)
        {
            x_vecs.push_back(vec(x_data[i][j]));
            y_vecs.push_back(vec(y_data[i][j]));
            z_vecs.push_back(vec(z_data[i][j]));
        }
        sd.x.push_back(x_vecs);
        sd.y.push_back(y_vecs);
        sd.z.push_back(z_vecs);
    }

    // Convert catheter data
    CatheterData cd;
    auto ch1_data = catheterData["ch1"].get<std::vector<std::vector<double>>>();
    auto ch2_data = catheterData["ch2"].get<std::vector<std::vector<double>>>();
    auto ch3_data = catheterData["ch3"].get<std::vector<std::vector<double>>>();

    cd.ch1 = mat(ch1_data.size(), ch1_data[0].size());
    cd.ch2 = mat(ch2_data.size(), ch2_data[0].size());
    cd.ch3 = mat(ch3_data.size(), ch3_data[0].size());

    for (size_t i = 0; i < ch1_data.size(); i++)
    {
        for (size_t j = 0; j < ch1_data[0].size(); j++)
        {
            cd.ch1(i, j) = ch1_data[i][j];
            cd.ch2(i, j) = ch2_data[i][j];
            cd.ch3(i, j) = ch3_data[i][j];
        }
    }

    // Call the implementation function
    return create_dose_matrix(PatientID, ct, sd, cd, static_cast<int>(angleResolution));
}

// Helper functions
inline mat createRotationMatrixX(double theta)
{
    double rx = theta * M_PI / 180.0;
    return mat{{1, 0, 0},
               {0, cos(rx), -sin(rx)},
               {0, sin(rx), cos(rx)}};
}

inline mat createRotationMatrixY(double theta)
{
    double ry = theta * M_PI / 180.0;
    return mat{{cos(ry), 0, sin(ry)},
               {0, 1, 0},
               {-sin(ry), 0, cos(ry)}};
}

inline mat createRotationMatrixZ(double theta)
{
    double rz = theta * M_PI / 180.0;
    return mat{{cos(rz), -sin(rz), 0},
               {sin(rz), cos(rz), 0},
               {0, 0, 1}};
}