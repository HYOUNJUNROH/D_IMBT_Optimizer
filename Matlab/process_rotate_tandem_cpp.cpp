#include "mex.hpp"
#include "mexAdapter.hpp"
#include "armadillo"
#include "NumCpp.hpp"
#include <thread>
#include <vector>

using namespace std;
using namespace arma;
using namespace matlab::mex;
using namespace matlab::engine;
using namespace matlab::data;

const size_t MAX_CONCURRENT = 8;

template<typename dtype>
tuple<Cube<dtype>, Cube<dtype>, Cube<dtype>> meshgrid(const nc::NdArray<dtype>& inICoords, const nc::NdArray<dtype>& inJCoords, const nc::NdArray<dtype>& inKCoords)
{
    STATIC_ASSERT_ARITHMETIC(dtype);

    const size_t numRows = inJCoords.size();
    const size_t numCols = inICoords.size();
    const size_t numSlices = inKCoords.size();

    auto returnArrayI = Cube<dtype>(numRows, numCols, numSlices);
    auto returnArrayJ = Cube<dtype>(numRows, numCols, numSlices);
    auto returnArrayK = Cube<dtype>(numRows, numCols, numSlices);

    // first the I array
    for (size_t row = 0; row < numRows; ++row)
    {
        for (size_t col = 0; col < numCols; ++col)
        {
            for (size_t slice = 0; slice < numSlices; ++slice)
            {
                returnArrayI(row, col, slice) = inICoords[col];
            }
        }
    }

    // then the J array
    for (size_t col = 0; col < numCols; ++col)
    {
        for (size_t row = 0; row < numRows; ++row)
        {
            for (size_t slice = 0; slice < numSlices; ++slice)
            {
                returnArrayJ(row, col, slice) = inJCoords[row];
            }
        }
    }

    // then the K array
    for (size_t col = 0; col < numCols; ++col)
    {
        for (size_t row = 0; row < numRows; ++row)
        {
            for (size_t slice = 0; slice < numSlices; ++slice)
            {
                returnArrayK(row, col, slice) = inKCoords[slice];
            }
        }
    }

    return make_tuple(returnArrayI, returnArrayJ, returnArrayK);
}

template<class T>
    Mat<T> getMat(TypedArray<T> A)
{
    TypedIterator<T> it = A.begin();
    ArrayDimensions nDim = A.getDimensions();
    return Mat<T>(it.operator->(), nDim[0], nDim[1]);
}

template<class T>
    Cube<T> getCube(TypedArray<T> A)
{
    TypedIterator<T> it = A.begin();
    ArrayDimensions nDim = A.getDimensions();
    return Cube<T>(it.operator->(), nDim[0], nDim[1], nDim[2]);
}

template<class T>
    uword length(Mat<T> matrix)
{
    return max(matrix.n_cols, matrix.n_rows);
}

class MexFunction : public Function {
    private:
    shared_ptr<MATLABEngine> matlabPtr = getEngine();
    ArrayFactory factory;
    ostringstream stream;

    mat extract_ind(const cube& newd, const mat& ind_vec)
    {
        mat extracted(size(ind_vec));
        size_t idx = 0;
        for (size_t ind : ind_vec) {
            extracted(idx) = newd(ind);
            idx++;
        }
        return extracted;
    }

    public:
    vector<Array> process_rotate_tandem(ArgumentList& args)
    {
        auto& Xc = getCube<double>(args[0]);
        auto& Yc = getCube<double>(args[1]);
        auto& Zc = getCube<double>(args[2]);
        auto& vC = getMat<double>(args[3]);
        auto& source_pos = getMat<double>(args[4]);
        auto& angle_pos = getMat<double>(args[5]);
        auto& flag_body = getMat<double>(args[6]);
        auto& ind_PTVd = getMat<double>(args[7]);
        auto& ind_bladderd = getMat<double>(args[8]);
        auto& ind_rectumd = getMat<double>(args[9]);
        auto& ind_boweld = getMat<double>(args[10]);
        auto& ind_sigmoidd = getMat<double>(args[11]);
        auto& ind_bodyd = getMat<double>(args[12]);
        auto& A_org = getCube<double>(args[13]);

        size_t n_items = angle_pos.n_elem * source_pos.n_rows;
        Mat<double> PTV(ind_PTVd.n_rows, n_items);
        Mat<double> bladder(ind_bladderd.n_rows, n_items);
        Mat<double> rectum(ind_rectumd.n_rows, n_items);
        Mat<double> bowel(ind_boweld.n_rows, n_items);
        Mat<double> sigmoid(ind_sigmoidd.n_rows, n_items);
        Mat<double> body(ind_bodyd.n_rows, n_items);

        Mat<double> sum_tandem = zeros(1,1);
        size_t cnt = 0;

        size_t n_threads = thread::hardware_concurrency();
        if (n_threads > MAX_CONCURRENT) {
            n_threads = MAX_CONCURRENT;
        }
        vector<thread> workers;
        workers.reserve(n_threads);

        size_t n_working = n_threads;
        for (size_t jj = 0; jj < length(angle_pos); ++jj) {
            for (size_t ii = 0; ii < source_pos.n_rows; ++ii) {
                auto A = A_org;
                workers.emplace_back(&MexFunction::task_rotate_tandem, this, PTV, bladder, rectum, bowel, sigmoid, body, sum_tandem, flag_body,
                       ind_PTVd, ind_bladderd, ind_rectumd, ind_boweld, ind_sigmoidd, ind_bodyd,
                       jj, ii, Xc, Yc, Zc, vC, source_pos, angle_pos, A, cnt);
                cnt++;
                cout << cnt << endl;

                n_working--;
                if (workers.size() > 0 && n_working == 0) {
                    n_working = n_threads;
                    for (auto& t : workers) t.join();
                    workers.clear();
                    cout << "t.join() && workers.clear()" << endl;
                }
            }
        }

        if (workers.size() > 0) {
            for (auto& t : workers) t.join();
            workers.clear();
            cout << "last t.join() && workers.clear()" << endl;
        }

        vector<Array> results(8);
        results[0] = factory.createArray<double>({PTV.n_rows, PTV.n_cols}, PTV.begin(), PTV.end());
        results[1] = factory.createArray<double>({bladder.n_rows, bladder.n_cols}, bladder.begin(), bladder.end());
        results[2] = factory.createArray<double>({rectum.n_rows, rectum.n_cols}, rectum.begin(), rectum.end());
        results[3] = factory.createArray<double>({bowel.n_rows, bowel.n_cols}, bowel.begin(), bowel.end());
        results[4] = factory.createArray<double>({sigmoid.n_rows, sigmoid.n_cols}, sigmoid.begin(), sigmoid.end());
        results[5] = factory.createArray<double>({body.n_rows, body.n_cols}, body.begin(), body.end());
        results[6] = factory.createScalar(cnt+1);
        results[7] = factory.createScalar(as_scalar(sum_tandem));

        return results;
    }

    void task_rotate_tandem(mat& PTV, mat& bladder, mat& rectum, mat& bowel, mat& sigmoid, mat& body, mat& sum_tandem, const mat& flag_body,
                            const mat& ind_PTVd, const mat& ind_bladderd, const mat& ind_rectumd, const mat& ind_boweld, const mat& ind_sigmoidd, const mat& ind_bodyd,
                            size_t jj, size_t ii, const cube& Xc, const cube& Yc, const cube& Zc, const mat& vC, const mat& source_pos, const mat& angle_pos, cube& A, size_t cnt)
    {
        auto result_tuple = rotate_tandem(jj, ii, Xc, Yc, Zc, vC, source_pos, angle_pos, A);
        auto Arot_newd = get<0>(result_tuple);
        auto Arot_newd_4 = get<1>(result_tuple);
        sum_tandem = get<2>(result_tuple);

        PTV.col(cnt) = extract_ind(Arot_newd, ind_PTVd);
        bladder.col(cnt) = extract_ind(Arot_newd, ind_bladderd);
        rectum.col(cnt) = extract_ind(Arot_newd, ind_rectumd);
        bowel.col(cnt) = extract_ind(Arot_newd, ind_boweld);
        sigmoid.col(cnt) = extract_ind(Arot_newd, ind_sigmoidd);
        if (as_scalar(flag_body) == 1) {
            body.col(cnt) = extract_ind(Arot_newd_4, ind_bodyd);
        }
    }

    tuple<cube, cube, mat> rotate_tandem(double jj, double ii, const cube& Xc, const cube& Yc, const cube& Zc, const mat& vC, const mat& source_pos, const mat& angle_pos, cube A)
    {
        double theta_x2 = 0.0;
        double theta_y2 = 0.0;
        double theta_z2 = 0.0;

        if (ii == 0) {
            theta_x2 = (atan((source_pos(ii,1)-source_pos(ii+1,1))/(source_pos(ii,2)-source_pos(ii+1,2)))*180/datum::pi);
            theta_y2 = (-atan((source_pos(ii,0)-source_pos(ii+1,0))/(source_pos(ii,2)-source_pos(ii+1,2)))*180/datum::pi);
        }
        else {
            theta_x2 = (atan((source_pos(ii-1,1)-source_pos(ii,1))/(source_pos(ii-1,2)-source_pos(ii,2)))*180/datum::pi);
            theta_y2 = (-atan((source_pos(ii-1,0)-source_pos(ii,0))/(source_pos(ii-1,2)-source_pos(ii,2)))*180/datum::pi);
        }
        theta_z2 = angle_pos(jj);

        auto xt = nc::arange<double>(source_pos(ii,0)-101, source_pos(ii,0)+100, 1);
        auto yt = nc::arange<double>(source_pos(ii,1)-101, source_pos(ii,1)+99, 1);
        auto zt = nc::arange<double>(source_pos(ii,2)+101, source_pos(ii,2)-100, -1);

        A = reshape(A, 201, 201, 201);
        auto ind_neg = find(A < 0);
        for (auto idx : ind_neg) {
            A(idx) = 0;
        }

        mat sum_tandem = zeros(1,1);
        sum_tandem(0,0) = accu(A);

        mat vA(1, 3);
        vA(0,0) = size(A).n_rows;
        vA(0,1) = size(A).n_cols;
        vA(0,2) = size(A).n_slices;

        auto Arot = rotate3D(A, theta_x2, theta_y2, theta_z2, vA);

        auto meshgrid_XYZ = meshgrid(xt, yt, zt);
        auto Xt = get<0>(meshgrid_XYZ);
        auto Yt = get<1>(meshgrid_XYZ);
        auto Zt = get<2>(meshgrid_XYZ);

        auto Arot_new = interp3_cpp(Xt, Yt, Zt, Arot, Xc, Yc, Zc);
        cube Arot_newd = zeros(vC(0)/2, vC(1)/2, vC(2));
        cube Arot_newd_4 = zeros(vC(0)/4, vC(1)/4, vC(2));

        for (auto ss = 0; ss < vC(2); ++ss) {
            for (auto rot_i = 0; rot_i < Arot_newd.n_rows; ++rot_i) {
                for (auto rot_j = 0; rot_j < Arot_newd.n_cols; ++rot_j) {
                    Arot_newd(rot_i, rot_j, ss) = Arot_new(rot_i*2, rot_j*2, ss);
                }
            }

            for (auto rot_i = 0; rot_i < Arot_newd_4.n_rows; ++rot_i) {
                for (auto rot_j = 0; rot_j < Arot_newd_4.n_cols; ++rot_j) {
                    Arot_newd_4(rot_i, rot_j, ss) = Arot_new(rot_i*4, rot_j*4, ss);
                }
            }
        }

        return make_tuple(Arot_newd, Arot_newd_4, sum_tandem);
    }

    cube rotate3D(cube& mask, double rot_x, double rot_y, double rot_z, mat& vC) {
        rotate_z(mask, vC, rot_z);
        rotate_x(mask, vC, rot_x);
        rotate_y(mask, vC, rot_y);
        return mask;
    }

    void rotate_z(cube& mask, const mat& vC, double rot_z) {
        for (int ii = 0; ii < vC(2); ii++) {
            mask.slice(ii) = imrotate_dec(static_cast<mat>(mask.slice(ii)), -rot_z);
        }
    }

    void rotate_x(cube& mask, const mat& vC, double rot_x) {
        for (int jj = 0; jj < vC(1); jj++) {
            mask.col(jj) = imrotate_dec(static_cast<mat>(mask.col(jj)), -rot_x);
        }
    }

    void rotate_y(cube& mask, const mat& vC, double rot_y) {
        for (int kk = 0; kk < vC(0); kk++) {
            mask.row(kk) = imrotate_dec(static_cast<mat>(mask.row(kk)), -rot_y);
        }
    }

    mat imrotate_dec(const mat& I, double deg) {
        auto [nc_y, nc_x] = nc::meshgrid(nc::arange<double>(nc::Slice(1, I.n_rows+1)), nc::arange<double>(nc::Slice(1, I.n_cols+1)));
        mat x(nc_x.data(), nc_x.numRows(), nc_x.numCols());
        mat y(nc_y.data(), nc_y.numRows(), nc_y.numCols());
        mat xy_mat = join_horiz(vectorise(x), vectorise(y));
        auto mid = mean(xy_mat, 0);
        xy_mat.each_row() -= mid;
        deg = deg * datum::pi / 180;
        mat R = { { cos(deg), sin(deg) }, { -sin(deg), cos(deg) } };
        mat xyrot_mat = xy_mat * R;
        cube xy(xy_mat.memptr(), x.n_rows, x.n_cols, 2, false);
        cube xyrot(xyrot_mat.memptr(), x.n_rows, x.n_cols, 2, false);

        return interp2_cpp(xy.slice(0), xy.slice(1), I, xyrot.slice(0), xyrot.slice(1));
    }

    cube interp3_cpp(const cube& X, const cube& Y, const cube& Z, const cube& V, const cube& Xq, const cube& Yq, const cube& Zq) {
        // TODO: need implement interp3_cpp
        cube temp_result = zeros(512, 512, 40);
        return temp_result;
    }

    mat interp2_cpp(const mat& X, const mat& Y, const mat& V, const mat& Xq, const mat& Yq) {
        mat result(V.n_rows, V.n_cols);
        interp2_F(X.memptr(), Y.memptr(), V.memptr(), V.n_rows, V.n_cols, Xq.memptr(), Yq.memptr(), V.n_rows*V.n_cols, result.memptr());
        return result;
    }

    void interp2_F(const double* const X,
                   const double* const Y,
                   const double* const data,
                   const size_t& nrows, const size_t& ncols,
                   const double* const Xq, const double* const Yq,
                   const size_t& N, double* result) {

        for (auto i = 0; i < N; ++i) {
            // get coordinates of corner location
            long long x_1 = static_cast<long long>(X[i]);
            long long x_2 = static_cast<long long>(Xq[i]);
            long long y_1 = static_cast<long long>(Y[i]);
            long long y_2 = static_cast<long long>(Yq[i]);

            // return 0 for target values that are out of bounds
            if (x_1 < 0 | x_2 > (nrows - 1) |  y_1 < 0 | y_2 > (ncols - 1)) {
                result[i] = datum::nan;
            } else {
                // get the array values
                const double& f_11 = data[x_1 + y_1*nrows];
                const double& f_12 = data[x_1 + y_2*nrows];
                const double& f_21 = data[x_2 + y_1*nrows];
                const double& f_22 = data[x_2 + y_2*nrows];

                // compute weights
                double w_x1 = x_2 - x_1;
                double w_x2 = x_2 - x_1;
                double w_y1 = y_2 - y_1;
                double w_y2 = y_2 - y_1;

                double a,b;
                a = f_11 * w_x1 + f_21 * w_x2;
                b = f_12 * w_x1 + f_22 * w_x2;
                result[i] = a * w_y1 + b * w_y2;
            }
        }
    }

    void operator()(ArgumentList outputs, ArgumentList inputs) {
        if (outputs.size() != 8) {
            matlabPtr->feval(u"error", 0, std::vector<matlab::data::Array>({factory.createScalar("8 outputs required")}));
            return;
        }

        if (inputs.size() != 14) {
            matlabPtr->feval(u"error", 0, std::vector<matlab::data::Array>({factory.createScalar("14 inputs required")}));
            return;
        }

        vector<Array> results = process_rotate_tandem(inputs);
        if (outputs.size() == results.size()) {
            copy(results.begin(), results.end(), outputs.begin());
        } else {
            matlabPtr->feval(u"error", 0, std::vector<matlab::data::Array>({factory.createScalar("results size is not same with outputs size")}));
            return;
        }
    }
};

