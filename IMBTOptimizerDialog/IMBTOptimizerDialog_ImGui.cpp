#include "IMBTOptimizerDialog_ImGui.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "stb_image.h"

#define NOMINMAX
#include <windows.h>
#undef NOMINMAX

#include "afxdialogex.h"
#include "CDialogIMBTOptimizer_ImGUI.h"

#ifndef USE_MATLAB_ENGINE
#include <armadillo>
#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

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

extern DoseMatrixResult create_dosematrix_pure_matlab_c__(
	const std::string &PatientID,
	const json &CTData,
	const json &StructData,
	const json &Catheter,
	double AngleResolution);

extern std::tuple<cube, vec> main_Brachy_GS_L0_patient2_ovoid_2022_matlab_c__(double PrescriptionDose, double Normalization, int AngleResolution,
																			  mat PTV, mat bladder, mat rectum, mat bowel, mat sigmoid, mat body, uvec ind_bodyd);
#endif

RTStruct IMBTOptimizerDialog_ImGui::RTStructInfoContainer{};
CT IMBTOptimizerDialog_ImGui::CTInfoContainer{};
CalculatedDose IMBTOptimizerDialog_ImGui::calculatedDoseContainer{};
RTDVH IMBTOptimizerDialog_ImGui::DVHInfoContainer{};
RTPlan IMBTOptimizerDialog_ImGui::RTPlanContainer{};
//std::vector<Constraints> IMBTOptimizerDialog_ImGui::constraintsContainer{};
static bool g_glfwInitialized = false;
static bool doseVoxelTextureInit;
using namespace std;

#ifdef USE_MATLAB_ENGINE
// static auto _createCellArray(std::vector<std::vector<std::vector<float>>> data_list);
static auto _createCellArray(std::vector<std::vector<std::vector<float>>> data_list)
{
	matlab::data::ArrayFactory factory;
	auto cell_array = factory.createCellArray({1, data_list.size()});
	auto list_size = data_list.size();

	for (size_t index_list = 0; index_list < list_size; ++index_list)
	{
		auto item_list_size = data_list[index_list].size();
		auto item_array = factory.createCellArray({1, item_list_size});
		for (size_t index_item = 0; index_item < item_list_size; ++index_item)
		{
			auto list_item = data_list[index_list][index_item];
			auto list_item_size = data_list[index_list][index_item].size();
			// item_array[0][index_item] = factory.createArray({ 1, list_item_size }, list_item.begin(), list_item.end());
			item_array[0][index_item] = factory.createArray({list_item_size, 1}, list_item.begin(), list_item.end());
		}
		cell_array[0][index_list] = item_array;
	}
	return cell_array;
}
#endif

IMBTOptimizerDialog_ImGui::IMBTOptimizerDialog_ImGui(
	CPlan* pPlan,
	CDialogIMBTOptimizer_ImGUI* _pDialog,
	bool bPlanModified,
	bool* _pCalculatedDoseConfirm
)
{
	m_pDialog = _pDialog;
	m_pCPlan = pPlan;
	m_CalculatedDoseConfirm = _pCalculatedDoseConfirm;
	m_bPlanModified = bPlanModified;
	m_pGUIWindow = nullptr;

	// 상태 1: Plan이 수정되었고 확정되지 않은 경우 - 완전 초기화
	if (m_bPlanModified || !*m_CalculatedDoseConfirm) {
		// 모든 컨테이너 완전 초기화
		initialize();

		// 완전히 초기화된 상태에서 시작
		calculatedDoseContainer.InitializedDoseKernel = false;
		calculatedDoseContainer.InitializationInProgress = false;
		calculatedDoseContainer.calculationInProcess = false;
		calculatedDoseContainer.calculatedDoseConfirm = false;
		calculatedDoseContainer.calculationComplete = false;

		DVHInfoContainer.calculatingDVH = false;
		DVHInfoContainer.DVHCalculated = false;

		originalDataConverted = false; // 데이터 변환 필요
	}
	// 상태 2: Plan 변경은 없으나 이전 계산이 확정되지 않은 경우 - 이전 단계 계속
	else if (!*m_CalculatedDoseConfirm) {
		// 부분 초기화: 필요한 컨테이너와 플래그만 초기화

		// 계산 상태 유지 (이전 단계에서 계속)
		calculatedDoseContainer.calculatedDoseConfirm = false;

		originalDataConverted = true; // 데이터는 이미 변환됨
	}
	// 상태 3: 계산이 확정된 경우 - 결과 표시, 새 계산 시 처음부터
	else {
		// 계산 결과 표시 상태
		calculatedDoseContainer.InitializedDoseKernel = true;
		calculatedDoseContainer.InitializationInProgress = false;
		calculatedDoseContainer.calculationInProcess = false;
		calculatedDoseContainer.calculatedDoseConfirm = true;
		calculatedDoseContainer.calculationComplete = true;

		DVHInfoContainer.calculatingDVH = false;
		DVHInfoContainer.DVHCalculated = true;

		originalDataConverted = true;
	}

	// CT, 윤곽선 새로고침 필요
	CTInfoContainer.refreshCTImageTexture = true;
	calculatedDoseContainer.refreshDoseTexture = false;
	RTStructInfoContainer.currentCTImageContourDataLoading = true;
	doseVoxelTextureInit = false;
}

IMBTOptimizerDialog_ImGui::~IMBTOptimizerDialog_ImGui()
{
//	initialize();
}

void IMBTOptimizerDialog_ImGui::initialize()
{
	// 1) Container 재할당
	IMBTOptimizerDialog_ImGui::RTStructInfoContainer = RTStruct{};
	IMBTOptimizerDialog_ImGui::CTInfoContainer = CT{};
	IMBTOptimizerDialog_ImGui::calculatedDoseContainer = CalculatedDose{};
	IMBTOptimizerDialog_ImGui::DVHInfoContainer = RTDVH{};
	IMBTOptimizerDialog_ImGui::RTPlanContainer = RTPlan{};
	//IMBTOptimizerDialog_ImGui::constraintsContainer = std::vector<Constraints>{};


	//CT,Struct refresh
	CTInfoContainer.refreshCTImageTexture = true;
	RTStructInfoContainer.currentCTImageContourDataLoading = true;
	//originalDataConverted = true;
}

void window_refresh_callback(GLFWwindow *window)
{
	glfwSwapBuffers(window);
	glFinish(); // important, this waits until rendering result is actually visible, thus making resizing less ugly
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void IMBTOptimizerDialog_ImGui::drawGUIWindow(GLuint *CTVoxelTexture, GLuint *doseVoxelTexture, CPlan *planData, OpenCLHandler* openCLHandler)
{
	if (originalDataConverted)
	{
		showPatientInfo(20, 10, windowSize.x - 900, 85);
		showOptimizationOptions(20 + windowSize.x - 900 + 25, 10, 825, 85);
		showCTVoxelAndDose(CTVoxelTexture, doseVoxelTexture, planData, 20, 110, windowSize.x - 900, windowSize.y - 380);
		showROIandDVHInfo(20 + windowSize.x - 900 + 25, 110, 825, 500, openCLHandler);
		showConstraintInfo(20 + windowSize.x - 900 + 25, 630, 825, 430);
		showDoseCalculationControlPanel(planData, 20, 110 + windowSize.y - 380 + 20, windowSize.x - 900, 230);
	}
}

#ifdef USE_MATLAB_ENGINE
bool IMBTOptimizerDialog_ImGui::showDialog() // Not used
{
	try
	{
		int displayW{};
		int displayH{};
		float contentScaleX = 1.0f;
		float contentScaleY = 1.0f;

		// GLFW Initialization
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		// glfwWindowHint(GLFW_SCALE_TO_MONITOR, 2); // 4K Monitor

		glfwSwapInterval(1);

		// Get Monitor Resolution
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);

		// windowSize.x = int(mode->width * 0.8);
		// windowSize.y = int(mode->height * 0.8);

		GLFWwindow *GUIWindow;
		GUIWindow = glfwCreateWindow(int(windowSize.x), int(windowSize.y), "IMBT Optimizer", nullptr, nullptr);
		glfwMakeContextCurrent(GUIWindow);

		// GL3W Initialization
		gl3wInit();

		// ImGui Initialization
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO();

		ImGui::StyleColorsLight();
		ImGuiStyle &style = ImGui::GetStyle();
		style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
		style.SelectableTextAlign = ImVec2(0.5f, 0.5f);
		io.IniFilename = nullptr;
		io.ConfigDragClickToInputText = true;

		ImGui_ImplGlfw_InitForOpenGL(GUIWindow, true);
		ImGui_ImplOpenGL3_Init("#version 460 core");

		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 10.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 12.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 16.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 20.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 24.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 28.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 32.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 36.f);

		static GLuint CTVoxelTexture{};
		glGenTextures(1, &CTVoxelTexture);
		glBindTexture(GL_TEXTURE_2D, CTVoxelTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Conversion of original data
		std::thread thread(&IMBTOptimizerDialog_ImGui::convertDataFromTPS, this, m_pCPlan);
		thread.detach();

		while (!glfwWindowShouldClose(GUIWindow))
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// ImGui::ShowDemoWindow();
			drawGUIWindow(&CTVoxelTexture, m_pCPlan);

			// Render whole components
			ImGui::Render();

			glfwMakeContextCurrent(GUIWindow);

			glfwGetFramebufferSize(GUIWindow, &displayW, &displayH);
			glfwGetWindowContentScale(GUIWindow, &contentScaleX, &contentScaleY);

			// Resize Callback
			glfwSetWindowRefreshCallback(GUIWindow, window_refresh_callback);
			glfwSetFramebufferSizeCallback(GUIWindow, framebuffer_size_callback);

			glViewport(0, 0, displayW, displayH);
			glClearColor(50.f / 255.f, 61.f / 255.f, 72.f / 255.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);

			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(GUIWindow);

			glfwPollEvents();
		}

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(GUIWindow);
		glfwTerminate();
	}
	catch (exception e)
	{
		showMessageBox("Error", "Can't open the Optimization Dialog");

		return false;
	}

	return true;
}
#endif

bool IMBTOptimizerDialog_ImGui::initGUI(HWND parentWindow)
{
#ifdef USE_MATLAB_ENGINE
	try
	{
		std::thread thread(&MatlabEngine::startMatlabEngine);
		thread.detach();
	}
	catch (exception e)
	{
		showMessageBox("Error", "MatlabEngine Error");

		return false;
	}
#endif

	try
	{
		int displayW{};
		int displayH{};
		float contentScaleX = 1.0f;
		float contentScaleY = 1.0f;

		// GLFW Initialization
		// 1) glfwInit()은 반드시 1회만 호출
		if (!g_glfwInitialized) {
			if (!glfwInit()) {
				MessageBoxA(nullptr, "Failed to init GLFW.", "Error", MB_OK);
				return false;
			}
			g_glfwInitialized = true;
		}

		// 2) GLFW 윈도우 설정
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		// glfwWindowHint(GLFW_SCALE_TO_MONITOR, 2); // 4K Monitor

		// Use vSync (sync with monitor Hz)
		glfwSwapInterval(1);

		// Get Monitor Resolution
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);

		m_pGUIWindow = glfwCreateWindow(int(windowSize.x), int(windowSize.y), "IMBT Optimizer", nullptr, nullptr);
		bool is_m_pGUIWindow = !m_pGUIWindow;
		if (is_m_pGUIWindow) {
			MessageBoxA(nullptr, "Failed to create GLFW window.", "Error", MB_OK);
			closeGUI(); // 혹시 초기화 된 것 있으면 정리
			return false;
		}
		glfwMakeContextCurrent(m_pGUIWindow);

		// Parent Window Size Change
		RECT windowRect, clientRect;
		GetWindowRect(parentWindow, &windowRect);
		GetClientRect(parentWindow, &clientRect);

		int pwHeight = windowSize.y + (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top) + 1;
		int pwWidth = windowSize.x + (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left) + 1;

		MoveWindow(parentWindow, (mode->width - pwWidth) / 2.0, (mode->height - pwHeight) / 2.0, pwWidth, pwHeight, true);

		// Get Handle of GUIWindow
		// 5) 부모 윈도우(parentWindow)에 임베드
		HWND hwndGLFWWindow = glfwGetWin32Window(m_pGUIWindow);
		SetParent(hwndGLFWWindow, parentWindow);

		// Change Style of embed GUIWindow
		// 스타일 변경 (Child Style)
		LONG nNewStyle = GetWindowLong(hwndGLFWWindow, GWL_STYLE) & ~WS_POPUP | WS_CHILDWINDOW;
		nNewStyle |= WS_CHILD;

		SetWindowLong(hwndGLFWWindow, GWL_STYLE, nNewStyle);

		ShowWindow(hwndGLFWWindow, SW_SHOW);

		// GL3W Initialization
		if (gl3wInit() != 0) {
			MessageBoxA(nullptr, "Failed to initialize OpenGL loader (gl3w).", "Error", MB_OK);
			closeGUI();
			return false;
		}

		// ImGui Initialization
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO();

		ImGui::StyleColorsLight();
		ImGuiStyle &style = ImGui::GetStyle();
		style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
		style.SelectableTextAlign = ImVec2(0.5f, 0.5f);
		io.IniFilename = nullptr;
		io.ConfigDragClickToInputText = true;

		ImGui_ImplGlfw_InitForOpenGL(m_pGUIWindow, true);
		ImGui_ImplOpenGL3_Init("#version 460 core");

		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 10.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 12.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 16.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 20.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 24.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 28.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 32.f);
		io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 36.f);
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Pre-texture generation for CT - 이미 텍스처가 생성되었는지 확인
		static GLuint CTVoxelTexture{};
		glGenTextures(1, &CTVoxelTexture);
		glBindTexture(GL_TEXTURE_2D, CTVoxelTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, 0);


		// Pre-texture generation for dose
		static GLuint doseVoxelTexture{};
		glGenTextures(1, &doseVoxelTexture);
		glBindTexture(GL_TEXTURE_2D, doseVoxelTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (!originalDataConverted)
		{
			// Conversion of original data
			std::thread dataLoadingThread(&IMBTOptimizerDialog_ImGui::convertDataFromTPS, this, m_pCPlan);
			dataLoadingThread.detach();
		}

		// Initialization of OpenCL
		OpenCLHandler openCLHandler;

		// 변경되었든 아니든 항상 이미지 갱신 플래그 설정
		CTInfoContainer.refreshCTImageTexture = true;
		RTStructInfoContainer.currentCTImageContourDataLoading = true;

		while (!glfwWindowShouldClose(m_pGUIWindow))
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// Parent Window Size Change
			GetWindowRect(parentWindow, &windowRect);
			GetClientRect(parentWindow, &clientRect);

			int currentWindowSize_y = (clientRect.bottom - clientRect.top) - 1;
			int currentWindowSize_x = (clientRect.right - clientRect.left) - 1;

			if (currentWindowSize_x != windowSize.x || currentWindowSize_y != windowSize.y)
			{
				windowSize.y = currentWindowSize_y;
				windowSize.x = currentWindowSize_x;
				RTStructInfoContainer.currentCTImageContourDataLoading = true; // recontour
				glfwSetWindowSize(m_pGUIWindow, windowSize.x, windowSize.y);
			}

			drawGUIWindow(&CTVoxelTexture, &doseVoxelTexture, m_pCPlan, &openCLHandler);

			// Render whole components
			ImGui::Render();

			glfwMakeContextCurrent(m_pGUIWindow);

			glfwGetFramebufferSize(m_pGUIWindow, &displayW, &displayH);
			glfwGetWindowContentScale(m_pGUIWindow, &contentScaleX, &contentScaleY);

			// Resize Callback
			// glfwSetWindowRefreshCallback(m_pGUIWindow, window_refresh_callback);
			// glfwSetFramebufferSizeCallback(m_pGUIWindow, framebuffer_size_callback);

			glViewport(0, 0, displayW, displayH);
			glClearColor(50.f / 255.f, 61.f / 255.f, 72.f / 255.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);

			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(m_pGUIWindow);

			glfwPollEvents();
		}

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		// 루프가 끝나면(즉, 창이 닫혀야 한다면) 종료 처리
		if (g_glfwInitialized) {
			glfwDestroyWindow(m_pGUIWindow);
			glfwTerminate();
			m_pGUIWindow = nullptr;
			g_glfwInitialized = false;
		}
	}
	catch (exception e)
	{
		showMessageBox("Error", "Can't open the Optimization Dialog");
		return false;
	}

	return true;
}

// 2. closeGUI() 함수 수정 - 텍스처 리소스 보존
bool IMBTOptimizerDialog_ImGui::closeGUI()
{
	if (g_glfwInitialized) {
		if (m_pGUIWindow == 0)
			return false;

		glfwSetWindowShouldClose(m_pGUIWindow, 1);

		if (!glfwWindowShouldClose(m_pGUIWindow))
			return false;

		// 텍스처 및 데이터는 삭제하지 않고 창만 닫음
		// g_glfwInitialized = false; // 이 부분 주석 처리
	}

	return true;
}

//---------------------------------------------------
// GUI functions
//---------------------------------------------------

// Display Patient Information and Prescription Dose
void IMBTOptimizerDialog_ImGui::showPatientInfo(float positionX, float positionY, float wWidth, float wHeight)
{
	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(positionX, positionY));
	ImGui::SetNextWindowSize(ImVec2(wWidth, wHeight));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0, 8.0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(46.f / 255.f, 73.f / 255.f, 125.f / 255.f, 1.f));
	ImGui::Begin("Patient information ", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();

	ImGui::PushFont(io.Fonts->Fonts[3]);

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
	ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "Name: %s", std::string(CT2CA(m_pCPlan->m_strName)).c_str());
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
	ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "ID: %s", std::string(CT2CA(m_pCPlan->m_strID)).c_str());
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
	ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "Prescription: %0.f cGy / %0.f fx", RTPlanContainer.prescriptionDose, RTPlanContainer.fractions);

	ImGui::PopFont();

	ImGui::End();
}

// .cpp 파일 상단(혹은 함수 근처)에서 전역(static) 배열로 정의
static const char* itemsAngleResolution[] = { "10", "20", "30", "40", "50", "60" };
static const int   NUM_ANGLE_ITEMS = sizeof(itemsAngleResolution) / sizeof(itemsAngleResolution[0]);

static const char* itemsNumDwellPositions[] = {
	"20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
	"30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40"
};
static const int   NUM_DWELL_ITEMS = sizeof(itemsNumDwellPositions) / sizeof(itemsNumDwellPositions[0]);
// Display Options of the optimizer
void IMBTOptimizerDialog_ImGui::showOptimizationOptions(float positionX, float positionY, float wWidth, float wHeight)
{
	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(positionX, positionY));
	ImGui::SetNextWindowSize(ImVec2(wWidth, wHeight));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0, 8.0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(46.f / 255.f, 73.f / 255.f, 125.f / 255.f, 1.f));
	ImGui::Begin("Optimization Info", nullptr, 
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoScrollbar | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoTitleBar);
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();

	// ------------------------------------
	// 1) 첫 호출에 한해, RTPlanContainer 값으로부터 인덱스 설정
	// ------------------------------------
	static bool sFirstCall = true;

	if ((sFirstCall == false) && (m_bPlanModified == true))
		sFirstCall = true;

	if (sFirstCall) {
		// angleResolution을 itemsAngleResolution 배열 안에서 찾아 sSelectedAngleIndex 설정
		for (int i = 0; i < NUM_ANGLE_ITEMS; i++) {
			if (std::stoi(itemsAngleResolution[i]) == RTPlanContainer.angleResolution) {
				sSelectedAngleIndex = i;
				break;
			}
		}
		// numOfDwellPositionTandem도 마찬가지
		for (int i = 0; i < NUM_DWELL_ITEMS; i++) {
			if (std::stoi(itemsNumDwellPositions[i]) == RTPlanContainer.numOfDwellPositionTandem) {
				sSelectedDwellIndex = i;
				break;
			}
		}
		sFirstCall = false;
	}

	// ------------------------------------
	// 2) 각 콤보박스: 인덱스로 표시 및 갱신
	// ------------------------------------
	ImGui::PushFont(io.Fonts->Fonts[3]);

	// ---- (A) 각도 해상도 콤보 ----
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
	ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "Angle Resolution:");

	ImGui::SameLine();

	ImGui::PushItemWidth(80);
	// 콤보의 '미리보기' 값은 itemsAngleResolution[sSelectedAngleIndex]
	if (ImGui::BeginCombo("##AngleResolution", itemsAngleResolution[sSelectedAngleIndex])) {
		for (int n = 0; n < NUM_ANGLE_ITEMS; n++) {
			// 현재 선택 여부
			bool is_selected = (sSelectedAngleIndex == n);

			if (ImGui::Selectable(itemsAngleResolution[n], is_selected)) {
				// 선택이 변경되었다면 sSelectedAngleIndex 갱신
				sSelectedAngleIndex = n;
				// RTPlanContainer도 함께 갱신
				RTPlanContainer.angleResolution = std::stoi(itemsAngleResolution[n]);

				if (calculatedDoseContainer.InitializedDoseKernel || calculatedDoseContainer.InitializationInProgress) {
					AfxMessageBox(_T("Optimization will be initialized"), MB_OK | MB_ICONWARNING);
				}

				//ImGUI버튼 관련
				calculatedDoseContainer.InitializedDoseKernel = false;
				calculatedDoseContainer.InitializationInProgress = false;
				calculatedDoseContainer.calculationInProcess = false;
				calculatedDoseContainer.calculatedDoseConfirm = false;
				//DVH 관련
				DVHInfoContainer.calculatingDVH = false;
				DVHInfoContainer.DVHCalculated = false;
				//doseVoxel reloding
				calculatedDoseContainer.calculationComplete = false;
				//CT,Struct refresh
				CTInfoContainer.refreshCTImageTexture = true;
				RTStructInfoContainer.currentCTImageContourDataLoading = true;
			}
			if (is_selected) {
				// 열렸을 때 자동 스크롤/포커스
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	// ---- (B) 텐덤 dwell positions 콤보 ----

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
	ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "The number of Tandem dwell Points:");

	ImGui::SameLine();

	ImGui::PushItemWidth(80);
	if (ImGui::BeginCombo("##NumDwellPositions", itemsNumDwellPositions[sSelectedDwellIndex])) {
		for (int n = 0; n < NUM_DWELL_ITEMS; n++) {
			bool is_selected = (sSelectedDwellIndex == n);

			if (ImGui::Selectable(itemsNumDwellPositions[n], is_selected)) {
				sSelectedDwellIndex = n;
				// RTPlanContainer도 함께 업데이트
				RTPlanContainer.numOfDwellPositionTandem = std::stoi(itemsNumDwellPositions[n]);

				if (calculatedDoseContainer.InitializedDoseKernel || calculatedDoseContainer.InitializationInProgress) {
					AfxMessageBox(_T("Optimization will be initialized"), MB_OK | MB_ICONWARNING);
				}
				
				//ImGUI버튼 관련
				calculatedDoseContainer.InitializedDoseKernel = false;
				calculatedDoseContainer.InitializationInProgress = false;
				calculatedDoseContainer.calculationInProcess = false;
				calculatedDoseContainer.calculatedDoseConfirm = false;
				//DVH 관련
				DVHInfoContainer.calculatingDVH = false;
				DVHInfoContainer.DVHCalculated = false;
				//doseVoxel reloding
				calculatedDoseContainer.calculationComplete = false;
				//CT,Struct refresh
				CTInfoContainer.refreshCTImageTexture = true;
				RTStructInfoContainer.currentCTImageContourDataLoading = true;
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	ImGui::PopFont();
	ImGui::End();
}

// World to screen coordinate conversion function
ImVec2 IMBTOptimizerDialog_ImGui::WorldToScreen(const ImVec2& worldPos, const ImVec2& canvasP0, const ImVec2& imageDimension, const ImVec2& imageSize, float zoom, const ImVec2& pan) 
{
	return ImVec2(
		(worldPos.x - pan.x * (imageDimension.x / imageSize.x)) * zoom + canvasP0.x,
		(worldPos.y - pan.y * (imageDimension.y / imageSize.y)) * zoom + canvasP0.y
	);
}

// CT voxel and dose information window
void IMBTOptimizerDialog_ImGui::showCTVoxelAndDose(GLuint *CTVoxelTexture, GLuint *doseVoxelTexture, CPlan *planData, float positionX, float positionY, float wWidth, float wHeight)
{
	ImVec2 CTWindowSize = {wWidth, wHeight};
	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(positionX, positionY));
	ImGui::SetNextWindowSize(CTWindowSize);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1.f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 1.f));

	ImGui::Begin("CT / dose window", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);

	/* CT image / dose view settings */
	float minWindowSize = std::min(CTWindowSize.x, CTWindowSize.y);
	ImVec2 CTImageSize = {minWindowSize, minWindowSize};
	ImVec2 CTimageStartCursorScreenPos = ImGui::GetCursorScreenPos();

	ImVec2 CTImagePosition;
	CTImagePosition.x = (CTWindowSize.x - CTImageSize.x) / 2.0;
	CTImagePosition.y = (CTWindowSize.y - CTImageSize.y) / 2.0;
	CTimageStartCursorScreenPos.x += CTImagePosition.x;
	CTimageStartCursorScreenPos.y += CTImagePosition.y;

	static int maxDoseScale; // Percentage
	static float prescriptionDoseValue; // Prescription dose
	static float maxDoseScaleValue; // 120% of Prescription Dose (cGy)
	static float minDoseScaleValue;	// 10% of Prescription Dose (cGy)

	// Mouse / cursor screen position to determine position of zooming
	static ImVec2 mousePos;
	static ImVec2 cursorScreenPos;

	// Zoom and pan status
	static bool zoomStatusOn; // If user uses zoom button
	static bool panStatusOn; // If user uses pan button

	// Zoom, pan value and view region size based on zoom and pan setting
	static float customViewZoom{ 1.0f };
	static ImVec2 customViewPan = ImVec2(0.0f, 0.0f);
	static ImVec2 customViewUV1 = ImVec2(0.0f, 0.0f);
	static ImVec2 customViewUV2 = ImVec2(1.0f, 1.0f);
	static ImVec2 displayMin;
	static ImVec2 displaySize;

	// Refresh CT image texture (After loading data and user uses mouse scroll)
	if (CTInfoContainer.refreshCTImageTexture)
	{
		// Data mapping for CT texture
		xt::xarray<std::uint8_t> dicomCTSliceXArray = xt::view(CTInfoContainer.textureVoxel, xt::range(CTInfoContainer.viewIndex, CTInfoContainer.viewIndex + 1), xt::all(), xt::all());
		dicomCTSliceXArray.reshape({ CTInfoContainer.dimension.x * CTInfoContainer.dimension.y });
		glBindTexture(GL_TEXTURE_2D, *CTVoxelTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, CTInfoContainer.dimension.x, CTInfoContainer.dimension.y, 0, GL_RED, GL_UNSIGNED_BYTE, dicomCTSliceXArray.begin());
		glBindTexture(GL_TEXTURE_2D, 0);
		CTInfoContainer.refreshCTImageTexture = false;
	}

	// Dose texture initialization (for the first time)
	if (!doseVoxelTextureInit)
	{
		std::vector<Color> initDoseImage(CTInfoContainer.dimension.x * CTInfoContainer.dimension.y);
		glBindTexture(GL_TEXTURE_2D, *doseVoxelTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, CTInfoContainer.dimension.x, CTInfoContainer.dimension.y, 0, GL_RGBA, GL_FLOAT, initDoseImage.data());
		glBindTexture(GL_TEXTURE_2D, 0);
		doseVoxelTextureInit = true;
	}

	// Refresh dose texture (If the absorbed dose was succssfully calculated and user uses mouses scrolls)
	if (calculatedDoseContainer.refreshDoseTexture)
	{
		// Set dose texture property
		maxDoseScale = 140; // Percentage
		prescriptionDoseValue = RTPlanContainer.prescriptionDose;
		maxDoseScaleValue = prescriptionDoseValue * maxDoseScale / 100.f; // 120% of Prescription Dose (cGy)
		minDoseScaleValue = prescriptionDoseValue * 0.3f; // 10% of Prescription Dose (cGy)

		// Data to color texture mapping
		std::vector<Color> doseImage(calculatedDoseContainer.dimension.x * calculatedDoseContainer.dimension.y);
		float transparence{};

		// Mapping absorbed dose value to RGBA value
		for (size_t i = 0; i < calculatedDoseContainer.dimension.y; i++)
		{
			for (size_t j = 0; j < calculatedDoseContainer.dimension.x; j++)
			{
				Color currentPixelColor;
				float doseValue = calculatedDoseContainer.doseVoxel.at(CTInfoContainer.viewIndex, i, j);
				if (doseValue >= minDoseScaleValue) transparence = 0.5f;
				else transparence = doseValue / minDoseScaleValue * 0.5f;
				currentPixelColor = getJetColorMap(doseValue, minDoseScaleValue, maxDoseScaleValue);
				currentPixelColor.A = transparence;
				doseImage[i * calculatedDoseContainer.dimension.x + j] = currentPixelColor;
			}
		}
		glBindTexture(GL_TEXTURE_2D, *doseVoxelTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, calculatedDoseContainer.dimension.x, calculatedDoseContainer.dimension.y, 0, GL_RGBA, GL_FLOAT, doseImage.data());
		glBindTexture(GL_TEXTURE_2D, 0);
		calculatedDoseContainer.refreshDoseTexture = false;
	}

	// Draw CT texutre
	ImGui::SetCursorPos(CTImagePosition);
	ImGui::Image((ImTextureID)*CTVoxelTexture, CTImageSize, customViewUV1, customViewUV2);

	// Draw dose texture
	ImGui::SetCursorPos(CTImagePosition);
	ImGui::Image((ImTextureID)*doseVoxelTexture, CTImageSize, customViewUV1, customViewUV2);

	// Get mouse position
	mousePos = ImGui::GetMousePos();
	//cursorScreenPos = ImGui::GetCursorScreenPos();

	// IO handling 
	if (ImGui::IsItemHovered() && !zoomStatusOn && !panStatusOn) // 1. No status
	{
		if (io.MouseWheel > 0)
		{
			CTInfoContainer.refreshCTImageTexture = true; // CT image texture refresh
			if (calculatedDoseContainer.calculationComplete) calculatedDoseContainer.refreshDoseTexture = true; // Dose image texture refresh
			RTStructInfoContainer.currentCTImageContourDataLoading = true; // Contour data reloading
			if (CTInfoContainer.viewIndex > 0) CTInfoContainer.viewIndex -= 1; // CT view index modification
		}
		else if (io.MouseWheel < 0)
		{
			CTInfoContainer.refreshCTImageTexture = true; // CT image texture refresh
			if (calculatedDoseContainer.calculationComplete) calculatedDoseContainer.refreshDoseTexture = true; // Dose image texture refresh
			RTStructInfoContainer.currentCTImageContourDataLoading = true; // Contour data reloading
			if (CTInfoContainer.viewIndex < CTInfoContainer.dimension.z - 1) CTInfoContainer.viewIndex += 1; // CT view index modification
		}
	}
	else if (ImGui::IsItemHovered() && zoomStatusOn) // 2. Zoom status on
	{
		// Modify zoom status
		if (ImGui::GetIO().MouseWheel != 0) 
		{
			float zoomDelta = ImGui::GetIO().MouseWheel * 0.1f;

			// Old value & new value of zoom
			float oldZoom = customViewZoom;
			float newZoomTemp = customViewZoom * std::pow(2.0f, zoomDelta);

			// Prohibit zoom range
			newZoomTemp = ImClamp(newZoomTemp, 1.0f, 4.0f);

			// Calculate relative mouse position
			ImVec2 positionDiff = ImVec2(mousePos.x - CTimageStartCursorScreenPos.x, mousePos.y - CTimageStartCursorScreenPos.y);

			// Calculate new pan position to zoom using mouse position
			ImVec2 newPanTemp = ImVec2(customViewPan.x + ((positionDiff.x / oldZoom) - (positionDiff.x / newZoomTemp)) / (CTImageSize.x / float(CTInfoContainer.dimension.x)), customViewPan.y + ((positionDiff.y / oldZoom) - (positionDiff.y / newZoomTemp)) / (CTImageSize.y / float(CTInfoContainer.dimension.y)));

			// If zoom / pan value is near threshold, initalize zoom and pan position
			if ((newZoomTemp < 1.4f && zoomDelta < 0) || ((customViewPan.x < 5 || customViewPan.y < 5) && (zoomDelta < 0)) || (((abs(newPanTemp.x + (CTInfoContainer.dimension.x / newZoomTemp) - CTInfoContainer.dimension.x) < 5) || (abs(newPanTemp.y + (CTInfoContainer.dimension.y / newZoomTemp) - CTInfoContainer.dimension.y) < 5)) && (zoomDelta < 0)))
			{
				customViewZoom = 1.0f;
				customViewPan = ImVec2(0.0f, 0.0f);
				customViewUV1 = ImVec2(0, 0);
				customViewUV2 = ImVec2(1, 1);
			}
			else
			{
				// Check that zoom modification triggers violation of view region limit
				bool limitNotReached = (newPanTemp.x + (CTInfoContainer.dimension.x / newZoomTemp) < CTInfoContainer.dimension.x) && (newPanTemp.y + (CTInfoContainer.dimension.y / newZoomTemp) < CTInfoContainer.dimension.y) && (newPanTemp.x > 0.f) && (newPanTemp.y > 0.f);

				// If there is no violation, modify zoom
				if (limitNotReached)
				{
					customViewZoom = newZoomTemp;
					customViewPan = newPanTemp;
					RTStructInfoContainer.contourDataRemappingForViewRegion = true;
				}
			}
		}
	}
	else if (ImGui::IsItemHovered() && panStatusOn) // 3. Pan status on
	{
		// Modifiy pan status
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) 
		{
			// Calculates pan limit
			float panLimitX = customViewPan.x + displaySize.x;
			float panLimitY = customViewPan.y + displaySize.y;

			ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;

			// Check that pan modification triggers violation of view region limit
			bool limitNotReached = (displaySize.x + (customViewPan.x - (mouse_delta.x / customViewZoom)) < CTInfoContainer.dimension.x) && (displaySize.y + (customViewPan.y - (mouse_delta.y / customViewZoom)) < CTInfoContainer.dimension.y) && (customViewPan.x - (mouse_delta.x / customViewZoom) > 0) && (customViewPan.y - (mouse_delta.y / customViewZoom) > 0);
			
			// If there is no violation, modify pan
			if (limitNotReached)
			{
				customViewPan.x -= mouse_delta.x / customViewZoom;
				customViewPan.y -= mouse_delta.y / customViewZoom;
				RTStructInfoContainer.contourDataRemappingForViewRegion = true;
			}
		}
	}

	// Zoom / pan status update to CT / dose image texture
	displayMin = customViewPan; // Display start area offset
	displaySize = ImVec2(CTInfoContainer.dimension.x / customViewZoom, CTInfoContainer.dimension.y / customViewZoom); // Display size
	customViewUV1 = ImVec2(displayMin.x / CTInfoContainer.dimension.x, displayMin.y / CTInfoContainer.dimension.y); // Start point of CT / dose image texture
	customViewUV2 = ImVec2((displayMin.x + displaySize.x) / CTInfoContainer.dimension.x, (displayMin.y + displaySize.y) / CTInfoContainer.dimension.y); // End point of CT / dose image texture

	/* ROI contour view */
	// Contour draw data loading (Every mousewheel IO)
	if (RTStructInfoContainer.currentCTImageContourDataLoading)
	{
		RTStructInfoContainer.CTImageAxialContourOriginPos.clear();
		RTStructInfoContainer.CTImageAxialContourOriginPos.resize(RTStructInfoContainer.CTImageAxialContourData[CTInfoContainer.viewIndex].size());

		for (int i = 0; i < RTStructInfoContainer.CTImageAxialContourData[CTInfoContainer.viewIndex].size(); i++)
		{
			std::vector<std::vector<ImVec2>> polygonPoints;
			for (int j = 0; j < RTStructInfoContainer.CTImageAxialContourData[CTInfoContainer.viewIndex][i].size(); j++)
			{
				std::vector<ImVec2> contourPoints;

				// Correction of contour point for drawing it in CT coordinate space
				for (int k = 0; k < RTStructInfoContainer.CTImageAxialContourData[CTInfoContainer.viewIndex][i][j].size(); k++)
				{
					// Convert contour points of world position to screen position (original position)
					float contourPointX = ((RTStructInfoContainer.CTImageAxialContourData[CTInfoContainer.viewIndex][i][j][k].x - CTInfoContainer.imagePositionPatients[CTInfoContainer.viewIndex].x) / CTInfoContainer.pixelSpacing.x) * (CTImageSize.x / float(CTInfoContainer.dimension.x));
					float contourPointY = ((RTStructInfoContainer.CTImageAxialContourData[CTInfoContainer.viewIndex][i][j][k].y - CTInfoContainer.imagePositionPatients[CTInfoContainer.viewIndex].y) / CTInfoContainer.pixelSpacing.y) * (CTImageSize.y / float(CTInfoContainer.dimension.y));
					contourPoints.push_back(ImVec2(contourPointX, contourPointY));
				}
				polygonPoints.push_back(contourPoints);
			}
			RTStructInfoContainer.CTImageAxialContourOriginPos[i] = polygonPoints;
		}
		RTStructInfoContainer.currentCTImageContourDataLoading = false;
	}

	// Renew contour points when user uses Zoom / pan
	if (RTStructInfoContainer.contourDataRemappingForViewRegion)
	{
		RTStructInfoContainer.CTImageAxialContourScreenPos = RTStructInfoContainer.CTImageAxialContourOriginPos;

		for (int i = 0; i < RTStructInfoContainer.CTImageAxialContourScreenPos.size(); i++)
		{
			for (int j = 0; j < RTStructInfoContainer.CTImageAxialContourScreenPos[i].size(); j++)
			{
				for (int k = 0; k < RTStructInfoContainer.CTImageAxialContourScreenPos[i][j].size(); k++)
				{
					RTStructInfoContainer.CTImageAxialContourScreenPos[i][j][k] = WorldToScreen(RTStructInfoContainer.CTImageAxialContourOriginPos[i][j][k], CTimageStartCursorScreenPos, CTImageSize, ImVec2(CTInfoContainer.dimension.x, CTInfoContainer.dimension.y), customViewZoom, customViewPan);
				}
			}
		}
	}

	// Draw contour polygons
	// Clip contour draw region for adjusting view from zoom/pan interaction 
	ImGui::GetWindowDrawList()->PushClipRect(CTimageStartCursorScreenPos, ImVec2(CTimageStartCursorScreenPos.x + CTImageSize.x, CTimageStartCursorScreenPos.y + CTImageSize.y), true);
	for (int i = 0; i < RTStructInfoContainer.CTImageAxialContourScreenPos.size(); i++)
	{
		if (RTStructInfoContainer.ROIPresentation[i] == true)
		{
			for (int j = 0; j < RTStructInfoContainer.CTImageAxialContourScreenPos[i].size(); j++)
			{
				ImGui::GetWindowDrawList()->AddPolyline(RTStructInfoContainer.CTImageAxialContourScreenPos[i][j].data(),
														int(RTStructInfoContainer.CTImageAxialContourScreenPos[i][j].size()),
														ImColor(ImVec4(RTStructInfoContainer.ROIColor[i].R, RTStructInfoContainer.ROIColor[i].G,
																	   RTStructInfoContainer.ROIColor[i].B, 1.f)),
														ImDrawFlags_Closed, 1.8f);
			}
		}
	}
	ImGui::GetWindowDrawList()->PopClipRect();

	// Dose calculation bar
	if (calculatedDoseContainer.calculationComplete)
	{
		// Draw dose scale bar
		ImVec2 doseScaleBarStartingPos = ImVec2(ImVec2(ImGui::GetWindowSize().x + positionX - 40, positionY + 20));
		ImGui::SetCursorPos(doseScaleBarStartingPos);

		for (int i = 0; i < 11; i++)
		{
			float value = maxDoseScaleValue * (1.f - i * 0.1f);
			ImVec4 colorMap = ImVec4(float(getJetColorMap(value, minDoseScaleValue, maxDoseScaleValue).R), float(getJetColorMap(value, minDoseScaleValue, maxDoseScaleValue).G), float(getJetColorMap(value, minDoseScaleValue, maxDoseScaleValue).B), 0.9f);
			ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(doseScaleBarStartingPos.x, doseScaleBarStartingPos.y + 15 * i), ImVec2(doseScaleBarStartingPos.x + 20.f, doseScaleBarStartingPos.y + 15 * (i + 1)), ImColor(colorMap));

			if (i == 0 || i == 2 || i == 4 || i == 6 || i == 8 || i == 10)
			{
				if (i == 0 || i == 2 || i == 4)
					ImGui::SetCursorPos(ImVec2(doseScaleBarStartingPos.x - 60.f, doseScaleBarStartingPos.y - positionY + 15.f * i - 3.f));
				else
					ImGui::SetCursorPos(ImVec2(doseScaleBarStartingPos.x - 50.f, doseScaleBarStartingPos.y - positionY + 15.f * i - 3.f));

				ImGui::PushFont(io.Fonts->Fonts[2]);
				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "%d%%", maxDoseScale - i * 10);
				ImGui::PopFont();
			}
		}
	}

	// Zoom button
	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 100, ImGui::GetWindowSize().y - 160));
	ImGui::PushFont(io.Fonts->Fonts[4]);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
	if (!zoomStatusOn)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 127.f / 255.f, 39.f / 255.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(221.f / 255.f, 88.f / 255.f, 0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(181.f / 255.F, 72.f / 255.f, 0.f, 1.f));
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(181.f / 255.F, 72.f / 255.f, 0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(181.f / 255.F, 72.f / 255.f, 0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(181.f / 255.F, 72.f / 255.f, 0.f, 1.f));
	}
	
	if (ImGui::Button("Zoom", ImVec2(80, 40)))
	{
		if (!zoomStatusOn)
		{
			zoomStatusOn = true;
			// If pan button is on, disable it
			if (panStatusOn) panStatusOn = false;
		}
		else zoomStatusOn = false;
	}
	ImGui::PopFont();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(4);

	// Pan button
	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 100, ImGui::GetWindowSize().y - 110));
	ImGui::PushFont(io.Fonts->Fonts[4]);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
	if (!panStatusOn)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 127.f / 255.f, 39.f / 255.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(221.f / 255.f, 88.f / 255.f, 0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(181.f / 255.F, 72.f / 255.f, 0.f, 1.f));
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(181.f / 255.F, 72.f / 255.f, 0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(181.f / 255.F, 72.f / 255.f, 0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(181.f / 255.F, 72.f / 255.f, 0.f, 1.f));
	}
	if (ImGui::Button("Pan", ImVec2(80, 40)))
	{
		if (!panStatusOn)
		{
			panStatusOn = true;
			// If zoom button is on, disable it
			if (zoomStatusOn) zoomStatusOn = false;
		}
		else panStatusOn = false;
	}
	ImGui::PopFont();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(4);

	// Initialize view button
	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 140, ImGui::GetWindowSize().y - 60));
	ImGui::PushFont(io.Fonts->Fonts[4]);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 127.f / 255.f, 39.f / 255.f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(221.f / 255.f, 88.f / 255.f, 0.f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(181.f / 255.F, 72.f / 255.f, 0.f, 1.f));
	if (ImGui::Button("Init view", ImVec2(120, 40)))
	{
		zoomStatusOn = false;
		panStatusOn = false;

		customViewZoom = 1.0f;
		customViewPan = ImVec2(0.0f, 0.0f);
		customViewUV1 = ImVec2(0, 0);
		customViewUV2 = ImVec2(1, 1);
	}
	ImGui::PopFont();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(4);

	// Pixel spacing indicator
	ImGui::SetCursorPos(ImVec2(15, ImGui::GetWindowSize().y - 70));
	ImGui::PushFont(io.Fonts->Fonts[4]);
	ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "Pixel spacing: %.2f mm", CTInfoContainer.pixelSpacing.z);
	ImGui::PopFont();

	// Index indicator
	ImGui::SetCursorPos(ImVec2(15, ImGui::GetWindowSize().y - 40));
	ImGui::PushFont(io.Fonts->Fonts[4]);
	ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "CT index: %d / %d", CTInfoContainer.viewIndex + 1, CTInfoContainer.dimension.z);
	ImGui::PopFont();

	ImGui::End();
}

// ROI information window
void IMBTOptimizerDialog_ImGui::showROIandDVHInfo(float positionX, float positionY, float wWidth, float wHeight, OpenCLHandler* openCLHandler)
{
	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(positionX, positionY));
	ImGui::SetNextWindowSize(ImVec2(wWidth, wHeight));
	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.f);
	ImGui::Begin("ROI/DVH window", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PopStyleVar();
	ImGui::PopFont();

	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 2.f);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

	// Temporal DVH calculation container
	static std::deque<bool> dvhCalculationSelection;
	static bool dvhCalculationSelectionToggleStatus;

	if (!DVHInfoContainer.calculatingDVH && !DVHInfoContainer.DVHCalculated && calculatedDoseContainer.calculationComplete)
	{
		// Temoral selection container initalization
		dvhCalculationSelection.clear();
		dvhCalculationSelection.resize(RTStructInfoContainer.ROINames.size());
		dvhCalculationSelectionToggleStatus = true;

		if (dvhCalculationSelection.size() > 0)
		{
			for (int i = 0; i < dvhCalculationSelection.size(); i++)
				dvhCalculationSelection[i] = true;

			// Copy temporary DVH calculation selection info
			DVHInfoContainer.DVHCalculationSelection.clear();
			DVHInfoContainer.DVHCalculationSelection.resize(dvhCalculationSelection.size());
			std::copy(dvhCalculationSelection.begin(), dvhCalculationSelection.end(), DVHInfoContainer.DVHCalculationSelection.begin());

			DVHInfoContainer.calculatingDVH = true;
			std::thread thread(&IMBTOptimizerDialog_ImGui::calculateDVH, this, openCLHandler);
			thread.detach();
		}
	}

	// Child window for ROI list
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

	// Display ROI list based on ROI name size
	ImGui::BeginChild("ROI list", ImVec2((wWidth - 25) / 3 - 20, wHeight - 90), ImGuiChildFlags_Border);
	for (int i = 0; i < RTStructInfoContainer.ROINames.size(); i++)
	{
		// Show ROI name
		ImGui::PushID(("Contour presentation toggle button " + std::to_string(i)).c_str());
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(RTStructInfoContainer.ROIColor.at(i).R, RTStructInfoContainer.ROIColor.at(i).G, RTStructInfoContainer.ROIColor.at(i).B, 1.f));
		ImGui::PushFont(io.Fonts->Fonts[4]);
		ImGui::Checkbox("", &RTStructInfoContainer.ROIPresentation[i]);
		ImGui::PopFont();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::PopID();

		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 13);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY());

		ImGui::PushFont(io.Fonts->Fonts[4]);
		ImGui::Text("%s", RTStructInfoContainer.ROINames.at(i).c_str());
		ImGui::PopFont();

		if (i < RTStructInfoContainer.ROINames.size() - 1)
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15);
	}

	ImGui::EndChild();

	ImGui::SameLine();

	// Plot display
	ImPlot::CreateContext();
	if (DVHInfoContainer.DVHCalculated)
	{
		ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(1.f, 1.f, 1.f, 1.f));
		ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0.95f, 0.95f, 0.95f, 1.f));
		ImPlot::PushStyleVar(ImPlotSubplotFlags_NoTitle, 0);
		ImPlot::SetNextAxesLimits(0.f, RTPlanContainer.prescriptionDose * 2.0f, 0.f, 120.f);
		if (ImPlot::BeginPlot("DVH", ImVec2((wWidth - 25) * 2 / 3 - 20, wHeight - 80), ImPlotFlags_NoInputs | ImPlotFlags_NoLegend | ImPlotFlags_NoTitle))
		{
			ImPlot::SetupAxes("Dose [cGy]", "Relative volume [%]");
			for (int i = 0; i < DVHInfoContainer.calculatedDVHcGyData.size(); i++)
			{
				if (RTStructInfoContainer.ROIPresentation[i])
				{
					ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(float(DVHInfoContainer.calculatedDVHROIColor[i].R), float(DVHInfoContainer.calculatedDVHROIColor[i].G), float(DVHInfoContainer.calculatedDVHROIColor[i].B), 1.f));
					ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 2.f);
					ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_None);
					ImPlot::PlotLine(DVHInfoContainer.calculatedDVHROIName[i].c_str(), &DVHInfoContainer.calculatedDVHcGyData[i][0], &DVHInfoContainer.calculatedDVHPercentData[i][0], int(DVHInfoContainer.calculatedDVHcGyData[i].size()), ImPlotFlags_None, 0, sizeof(double));
					ImPlot::PopStyleColor();
					ImPlot::PopStyleVar();
				}
			}
			ImPlot::EndPlot();
		}
		ImPlot::PopStyleColor(2);
	}
	else if (DVHInfoContainer.calculatingDVH)
	{
		ImGui::BeginChild("CalculatingDVH", ImVec2((wWidth - 25) * 2 / 3 - 20, wHeight - 90), ImGuiChildFlags_Border);
		{
			ImGui::PushFont(io.Fonts->Fonts[4]);

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 120);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowSize().x - ImGui::CalcTextSize("Calculating DVH. Please wait..").x) / 2);
			ImGui::Text("Calculating DVH. Please wait..");

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowSize().x - ImGui::CalcTextSize(("(" + std::to_string(dvhCalculationIndex) + "/" + std::to_string(RTStructInfoContainer.ROINames.size()) + ")").c_str()).x) / 2);
			ImGui::Text("%s", ("(" + std::to_string(dvhCalculationIndex) + "/" + std::to_string(RTStructInfoContainer.ROINames.size()) + ")").c_str());

			ImGui::PopFont();
		}
		ImGui::EndChild();
	}
	else
	{
		ImPlot::SetNextAxesLimits(0, 100, 0, 100);
		ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(1.f, 1.f, 1.f, 1.f));
		ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0.95f, 0.95f, 0.95f, 1.f));
		if (ImPlot::BeginPlot("DVH", ImVec2((wWidth - 25) * 2 / 3 - 20, wHeight - 80), ImPlotFlags_NoInputs | ImPlotFlags_NoLegend | ImPlotFlags_NoTitle))
		{
			ImPlot::SetupAxes("Dose [cGy]", "Relative volume [%]");
			ImPlot::EndPlot();
		}
		ImPlot::PopStyleColor(2);
	}
	ImPlot::DestroyContext();

	ImGui::PopStyleVar();
	ImGui::PopFont();

	ImGui::End();
}

// Constraint information window
void IMBTOptimizerDialog_ImGui::showConstraintInfo(float positionX, float positionY, float wWidth, float wHeight)
{
	ImGuiIO &io = ImGui::GetIO();

	// Constraints option container
	static std::array<std::string, 3> contraintType{"Upper", "Lower", "Target gEUD" };

	ImGui::SetNextWindowPos(ImVec2(positionX, positionY));
	ImGui::SetNextWindowSize(ImVec2(wWidth, wHeight));
	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.f);
	ImGui::Begin("DVH constraint window", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PopStyleVar();
	ImGui::PopFont();

	// Add constaints button
	ImGui::SetCursorPos(ImVec2(15, 40));
	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
	if (ImGui::Button("Add constraint", ImVec2(390, 40)))
	{
		// Constraints data initialization
		Constraints newContraints;
		newContraints.index = int(constraintsContainer.size());
		constraintsContainer.push_back(newContraints);
	}
	ImGui::PopStyleVar();
	ImGui::PopFont();

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

	// Delete last constraint information button
	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
	if (ImGui::Button("Delete last constraint", ImVec2(395, 40)))
	{
		if (constraintsContainer.size() > 0) constraintsContainer.pop_back();
	}
	ImGui::PopFont();
	ImGui::PopStyleVar();

	// Add constraint sub information button
	static int constraintIdx;
	static int constraintIdxMax;
	constraintIdxMax = constraintsContainer.size() - 1;

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

	ImGui::PushFont(io.Fonts->Fonts[3]);
	if (constraintsContainer.size() == 0) ImGui::BeginDisabled();
	ImGui::SetNextItemWidth(390);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 10.0f));
	ImGui::PushID("constraint sub info addition index slider");
	ImGui::SliderInt("", &constraintIdx, 0, constraintIdxMax, "ROI constraint idx : %d");
	ImGui::PopID();
	ImGui::PopStyleVar();

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
	if (ImGui::Button("Add constraints sub information", ImVec2(395, 40)))
	{
		ConstraintsSubInfo newContraintSubInfo;
		constraintsContainer[constraintIdx].constraintSubInfo.push_back(newContraintSubInfo);
	}
	ImGui::PopFont();
	ImGui::PopStyleVar();

	if (constraintsContainer.size() == 0) ImGui::EndDisabled();

	// Constraints information and setting header table 
	ImGui::SetCursorPos(ImVec2(15, ImGui::GetCursorPosY() + 10));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
	ImGui::PushFont(io.Fonts->Fonts[3]);
	if (ImGui::BeginTable("Constraints header table", 7, ImGuiTableFlags_PreciseWidths | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersInnerH, ImVec2(790, 260)))
	{
		// Freeze first row
		ImGui::TableSetupScrollFreeze(0, 1);

		// Set table column width
		ImGui::TableSetupColumn("Column 1", ImGuiTableColumnFlags_WidthFixed, 50.f);
		ImGui::TableSetupColumn("Column 2", ImGuiTableColumnFlags_WidthFixed, 200.f);
		ImGui::TableSetupColumn("Column 3", ImGuiTableColumnFlags_WidthFixed, 100.f);
		ImGui::TableSetupColumn("Column 4", ImGuiTableColumnFlags_WidthFixed, 100.f);
		ImGui::TableSetupColumn("Column 5", ImGuiTableColumnFlags_WidthFixed, 115.f);
		ImGui::TableSetupColumn("Column 6", ImGuiTableColumnFlags_WidthFixed, 115.f);
		ImGui::TableSetupColumn("Column 7", ImGuiTableColumnFlags_WidthFixed, 110.f);

		ImGui::TableNextRow(ImGuiTableRowFlags_None, 70.f);

		// Fisrt header column
		ImGui::TableNextColumn();
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor(210, 210, 210, 255));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize("Idx").x) / 2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (70.f - ImGui::CalcTextSize("Idx").y) / 2); 
		ImGui::Text("Idx");

		// Second header column
		ImGui::TableNextColumn();
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor(210, 210, 210, 255));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize("ID/Type").x) / 2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (70.f - ImGui::CalcTextSize("ID/Type").y) / 2);
		ImGui::Text("ID/Type");

		// Third header column
		ImGui::TableNextColumn();
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor(210, 210, 210, 255));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize("cm3").x) / 2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (70.f - ImGui::CalcTextSize("cm3").y) / 2);
		ImGui::Text("cm3");

		// Fourth header column
		ImGui::TableNextColumn();
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor(210, 210, 210, 255));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize("Vol [%]").x) / 2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (70.f - ImGui::CalcTextSize("Vol [%]").y) / 2);
		ImGui::Text("Vol [%%]");

		// Fifth header column
		ImGui::TableNextColumn();
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor(210, 210, 210, 255));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize("Dose [cGy]").x) / 2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (70.f - ImGui::CalcTextSize("Dose [cGy]").y) / 2);
		ImGui::Text("Dose [cGy]");

		// Sixth header column
		ImGui::TableNextColumn();
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor(210, 210, 210, 255));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize("Actual dose").x) / 2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10); 
		ImGui::Text("Actual dose");
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize("[cGy]").x) / 2);
		ImGui::Text("[cGy]");

		// Seventh header column
		ImGui::TableNextColumn();
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor(210, 210, 210, 255));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize("Priority").x) / 2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (70.f - ImGui::CalcTextSize("Priority").y) / 2);
		ImGui::Text("Priority");

		/* Contents */
		for (int i = 0; i < constraintsContainer.size(); i++)
		{
			// Constraint information
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 60.f);

			// Constraint index
			ImGui::TableNextColumn();
			ImGui::PushFont(io.Fonts->Fonts[3]);
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(std::to_string(i).c_str()).x) / 2, ImGui::GetCursorPosY() + (60.f - ImGui::CalcTextSize(std::to_string(i).c_str()).y) / 2));
			ImGui::Text("%d", constraintsContainer[i].index);
			ImGui::PopFont();

			// Constraint ID
			ImGui::TableNextColumn();
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 8, ImGui::GetCursorPosY() + 18));

			ImGui::SetNextItemWidth(184);
			ImGui::PushID(("Constraint ID (ROI) combo c-" + std::to_string(i)).c_str());
			if (ImGui::BeginCombo("", RTStructInfoContainer.ROINames[constraintsContainer[i].ROINameIndex].c_str(), ImGuiComboFlags_None))
			{
				for (int j = 0; j < RTStructInfoContainer.ROINames.size(); j++)
				{
					bool isSelected = (constraintsContainer[i].ROINameIndex == j);
					if (ImGui::Selectable(RTStructInfoContainer.ROINames[j].c_str(), isSelected)) constraintsContainer[i].ROINameIndex = j;
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::PopID();

			// Constraint ROI volume
			ImGui::TableNextColumn();
			ImGui::PushFont(io.Fonts->Fonts[3]);

			//if (DVHInfoContainer.DVHCalculated)
			//{
			//	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(DVHInfoContainer.calculatedVolumeSizeString[constraintsContainer[i].ROINameIndex].c_str()).x) / 2);
			//	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (60.f - ImGui::CalcTextSize(DVHInfoContainer.calculatedVolumeSizeString[constraintsContainer[i].ROINameIndex].c_str()).y) / 2);
			//	ImGui::Text(u8"%s", DVHInfoContainer.calculatedVolumeSizeString[constraintsContainer[i].ROINameIndex]);
			//}
			if (DVHInfoContainer.DVHCalculated)
			{
				// 디버그 확인용 
				const std::string& valueStr = DVHInfoContainer.calculatedVolumeSizeString[constraintsContainer[i].ROINameIndex];
				OutputDebugStringA(("Value to display: " + valueStr + "\n").c_str());

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(valueStr.c_str()).x) / 2);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (60.f - ImGui::CalcTextSize(valueStr.c_str()).y) / 2);
				ImGui::Text("%s", valueStr.c_str());
			}
			else
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize("N/A").x) / 2);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (60.f - ImGui::CalcTextSize("N/A").y) / 2);
				ImGui::Text("N/A");
			}
			ImGui::PopFont();


			// Current constraint sub information
			for (int j = 0; j < constraintsContainer[i].constraintSubInfo.size(); j++)
			{
				ImGui::TableNextRow(ImGuiTableRowFlags_None, 60.f);
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImColor(225, 225, 225, 255));

				// 1. ## Index skip

				// 2. Type
				ImGui::TableSetColumnIndex(1);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 8, ImGui::GetCursorPosY() + 18));

				ImGui::SetNextItemWidth(184);
				ImGui::PushID(("Constraint ID (ROI) c-" + std::to_string(i) + ", sub-" + std::to_string(j) + "type").c_str());
				if (ImGui::BeginCombo("", contraintType[constraintsContainer[i].constraintSubInfo[j].idType].c_str(), ImGuiComboFlags_None))
				{
					for (int k = 0; k < contraintType.size(); k++)
					{
						bool isSelected = (constraintsContainer[i].constraintSubInfo[j].idType == k);
						if (ImGui::Selectable(contraintType[k].c_str(), isSelected)) constraintsContainer[i].constraintSubInfo[j].idType = k;
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				ImGui::PopID();

				// 3. Constraint volume size (calculate using ROI volume size * volume percent)
				ImGui::TableSetColumnIndex(2);

				// 4. Constraint volume percent of original ROI
				ImGui::TableSetColumnIndex(3);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - 80.f) / 2, ImGui::GetCursorPosY() + 18));
				ImGui::SetNextItemWidth(80);
				ImGui::PushID(("Constraint ID (ROI) c-" + std::to_string(i) + ", sub-" + std::to_string(j) + "volume percent").c_str());
				ImGui::InputDouble("", &constraintsContainer[i].constraintSubInfo[j].constraintVolumePercent, 0.0, 0.0, "%.1f", ImGuiInputTextFlags_None);
				ImGui::PopID();

				// 5. Constraint dose [cGy]
				ImGui::TableSetColumnIndex(4);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - 99.f) / 2, ImGui::GetCursorPosY() + 18));
				ImGui::SetNextItemWidth(99);
				ImGui::PushID(("Constraint ID (ROI) c-" + std::to_string(i) + ", sub-" + std::to_string(j) + "dose").c_str());
				ImGui::InputInt("", &constraintsContainer[i].constraintSubInfo[j].constraintDose, 0, 0, ImGuiInputTextFlags_None);
				ImGui::PopID();

				// 6. Constraint actual dose [cGy]
				ImGui::TableSetColumnIndex(5);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - 99.f) / 2, ImGui::GetCursorPosY() + 18));
				ImGui::SetNextItemWidth(99);
				ImGui::PushID(("Constraint ID (ROI) c-" + std::to_string(i) + ", sub-" + std::to_string(j) + "actual dose").c_str());
				ImGui::InputInt("", &constraintsContainer[i].constraintSubInfo[j].constraintActualDose, 0, 0, ImGuiInputTextFlags_None);
				ImGui::PopID();

				// 7. Constraint priority
				ImGui::TableSetColumnIndex(6);
				ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - 94) / 2, ImGui::GetCursorPosY() + 18));
				ImGui::SetNextItemWidth(94);
				ImGui::PushID(("Constraint ID (ROI) c-" + std::to_string(i) + ", sub-" + std::to_string(j) + "priority").c_str());
				ImGui::InputInt("", &constraintsContainer[i].constraintSubInfo[j].constraintPriority, 0, 0, ImGuiInputTextFlags_None);
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::End();
}

void IMBTOptimizerDialog_ImGui::showDoseCalculationControlPanel(CPlan *planData, float positionX, float positionY, float wWidth, float wHeight)
{
	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(positionX, positionY));
	ImGui::SetNextWindowSize(ImVec2(wWidth, wHeight));

	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.f);
	ImGui::Begin("Dose calculation / optimization control panel window", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

	// Dose calculation result initalization
	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);

	if (calculatedDoseContainer.InitializedDoseKernel || calculatedDoseContainer.InitializationInProgress)
		ImGui::BeginDisabled();
	if (ImGui::Button("Initialize", ImVec2(235, 45)))
	{
		if (!calculatedDoseContainer.InitializedDoseKernel)
		{
			std::thread thread(&IMBTOptimizerDialog_ImGui::initializeDoseKernel, this, m_pCPlan);
			thread.detach();
		}
	}
	if (calculatedDoseContainer.InitializedDoseKernel || calculatedDoseContainer.InitializationInProgress)
		ImGui::EndDisabled();

	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

	// Dose calculation start button
	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
	// if (calculatedDoseContainer.calculationInProcess) ImGui::BeginDisabled();
	if (!calculatedDoseContainer.InitializedDoseKernel || calculatedDoseContainer.calculationInProcess)
		ImGui::BeginDisabled();
	if (ImGui::Button("Optimization Start", ImVec2(235, 45)))
	{
		//***** send constraint data for optimization, and clear
		std::thread thread(&IMBTOptimizerDialog_ImGui::optimizeIMBT_Matlab, this, m_pCPlan);
		thread.detach();
	}
	if (!calculatedDoseContainer.InitializedDoseKernel || calculatedDoseContainer.calculationInProcess)
		ImGui::EndDisabled();
	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

	// Dose calculation result confirm window
	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
	if (!calculatedDoseContainer.calculationComplete || calculatedDoseContainer.calculatedDoseConfirm)
		ImGui::BeginDisabled();
	if (ImGui::Button("Confirm", ImVec2(235, 45)))
	{
		if (calculatedDoseContainer.calculationComplete && calculatedDoseContainer.calculationInProcess == false)
		{
			std::thread thread(&IMBTOptimizerDialog_ImGui::ConfirmOptimization, this, m_pCPlan);
			thread.detach();
		}
	}
	if (!calculatedDoseContainer.calculationComplete || calculatedDoseContainer.calculatedDoseConfirm)
		ImGui::EndDisabled();
	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);

	// Dose calculation process cancellation
	ImGui::PushFont(io.Fonts->Fonts[3]);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
	if (!calculatedDoseContainer.calculationInProcess)
		ImGui::BeginDisabled();
	if (ImGui::Button("Cancel optimization", ImVec2(235, 45)))
	{
		/* ********** Insert abort function ********** */
	}
	if (!calculatedDoseContainer.calculationInProcess)
		ImGui::EndDisabled();
	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);

	ImGui::PushFont(io.Fonts->Fonts[2]);
	ImGui::BeginChild("Dose calculation optimization window", ImVec2(wWidth - 25, wHeight - 105), ImGuiChildFlags_Border);

	for (int i = 0; i < m_sOptimizationLog.size(); i++)
	{
		ImGui::Text(m_sOptimizationLog[i].c_str());
	}

	ImGui::EndChild();
	ImGui::PopFont();

	ImGui::End();
}

void IMBTOptimizerDialog_ImGui::convertDataFromTPS(CPlan *planData)
{
	// plan data need to be changed for real data **************************
	// *********************************************************************

	if (planData != nullptr)
	{
		/* 1. CT */
		// Get dimension of CT
		xt::xarray<size_t>::shape_type CTDimension = {size_t(planData->m_pPlannedCTData->m_anDim[2]), size_t(planData->m_pPlannedCTData->m_anDim[0]), size_t(planData->m_pPlannedCTData->m_anDim[1])};
		CTInfoContainer.dimension.z = CTDimension.at(0);
		CTInfoContainer.dimension.x = CTDimension.at(1);
		CTInfoContainer.dimension.y = CTDimension.at(2);

		// Get image position patient in 3D Volume
		CTInfoContainer.imagePositionPatient.x = planData->m_pPlannedCTData->m_afPatOrg[0];
		CTInfoContainer.imagePositionPatient.y = planData->m_pPlannedCTData->m_afPatOrg[1];
		CTInfoContainer.imagePositionPatient.z = planData->m_pPlannedCTData->m_afPatOrg[2];

		// Get image position patient per Slice
		for (int i = 0; i < CTInfoContainer.dimension.z; i++)
		{
			PointFloat3D ipp;

			ipp.x = planData->m_pPlannedCTData->m_ptsPatOrg[i].fx;
			ipp.y = planData->m_pPlannedCTData->m_ptsPatOrg[i].fy;
			ipp.z = planData->m_pPlannedCTData->m_ptsPatOrg[i].fz;
			ipp.z_sliceIndex = i;
			ipp.strReferencedSOPInstanceUID = planData->m_pPlannedCTData->m_arrSOPInstanceUID[i];

			CTInfoContainer.imagePositionPatients.push_back(ipp);
		}

		// Get orientation patient (Vertical / Horizontal)
		CTInfoContainer.imageOrientationPatientVer.x = int(planData->m_pPlannedCTData->m_afPatOrientVer[0]);
		CTInfoContainer.imageOrientationPatientVer.y = int(planData->m_pPlannedCTData->m_afPatOrientVer[1]);
		CTInfoContainer.imageOrientationPatientVer.z = int(planData->m_pPlannedCTData->m_afPatOrientVer[2]);

		CTInfoContainer.imageOrientationPatientHor.x = int(planData->m_pPlannedCTData->m_afPatOrientHor[0]);
		CTInfoContainer.imageOrientationPatientHor.y = int(planData->m_pPlannedCTData->m_afPatOrientHor[1]);
		CTInfoContainer.imageOrientationPatientHor.z = int(planData->m_pPlannedCTData->m_afPatOrientHor[2]);

		// Get pixel spacing of CT
		CTInfoContainer.pixelSpacing.x = planData->m_pPlannedCTData->m_afRes[0];
		CTInfoContainer.pixelSpacing.y = planData->m_pPlannedCTData->m_afRes[1];
		CTInfoContainer.pixelSpacing.z = planData->m_pPlannedCTData->m_afRes[2];

		CTInfoContainer.m_huCenter = planData->m_pPlannedCTData->m_nWndCenter;
		CTInfoContainer.m_huWidth = planData->m_pPlannedCTData->m_nWndWidth;

		// Plan Information
		RTPlanContainer.prescriptionDose = planData->m_fPrescriptionDosePerFraction * planData->m_fPlannedFraction;
		RTPlanContainer.fractions = planData->m_fPlannedFraction;
		RTPlanContainer.normalization = 90;

		// Get CT volume from plan data
		std::vector<float> CTVolumeVector;

		for (int i = 0; i < CTInfoContainer.dimension.z; i++)
		{
			for (int ny = 0; ny < CTInfoContainer.dimension.y; ny++)
			{
				for (int nx = 0; nx < CTInfoContainer.dimension.x; nx++)
				{
					float hu = (planData->m_pPlannedCTData->m_ppfHU)[i][ny * CTInfoContainer.dimension.y + nx];

					CTVolumeVector.push_back(hu);
				}
			}
		}

		// Reshape original CT voxel with dimension information
		CTInfoContainer.originalCTVoxel.resize({CTDimension[0] * CTDimension[1] * CTDimension[2]});
		CTInfoContainer.textureVoxel.resize({CTDimension[0] * CTDimension[1] * CTDimension[2]});

		// Copy std::vector info to xt::xarray
		CTInfoContainer.originalCTVoxel = xt::adapt(CTVolumeVector, {CTDimension[0] * CTDimension[1] * CTDimension[2]});

		// Create CT texture voxel array and convert it with conversion function (uint8_t)
		std::transform(CTInfoContainer.originalCTVoxel.cbegin(), CTInfoContainer.originalCTVoxel.cend(), CTInfoContainer.textureVoxel.begin(), [=](auto &&v)
					   { return HUtoTextureMapping(v, CTInfoContainer); });

		CTInfoContainer.originalCTVoxel.reshape(CTDimension);
		CTInfoContainer.textureVoxel.reshape(CTDimension);

		// Clean up
		CTVolumeVector.clear();
		CTVolumeVector.shrink_to_fit();

		/* 2. Contour (ROI) */
		// Get ROI item size
		size_t ROIItemSize = planData->m_pPlannedStructData->m_vecROIItems.size();

		// Reserve the data buffer
		RTStructInfoContainer.ROIContour.reserve(ROIItemSize);

		// Load data from original data buffer
		for (unsigned int i = 0; i < ROIItemSize; i++)
		{
			// Get ROI name
			RTStructInfoContainer.ROINames.push_back(std::string(CT2CA(planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_strName)));

			mt19937 randomEngine1((unsigned int)time(NULL) + i + 1);
			mt19937 randomEngine2((unsigned int)time(NULL) + i + 2);
			mt19937 randomEngine3((unsigned int)time(NULL) + i + 3);
			uniform_int_distribution<int> distributionRange(0, 255);
			auto generator1 = bind(distributionRange, randomEngine1);
			auto generator2 = bind(distributionRange, randomEngine2);
			auto generator3 = bind(distributionRange, randomEngine3);

			// Get ROI color
			Color ROIColor;
			ROIColor.R = float(generator1()) / 255.f; // float(planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_rgbColor.rgbRed) / 255.f;
			ROIColor.G = float(generator2()) / 255.f; // float(planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_rgbColor.rgbGreen) / 255.f;
			ROIColor.B = float(generator3()) / 255.f; // float(planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_rgbColor.rgbBlue) / 255.f;
			RTStructInfoContainer.ROIColor.push_back(ROIColor);

			// Define a ROI contour container
			// Reserve the data buffer
			std::vector<std::vector<PointFloat3D>> ROIContourContainer;
			size_t ROIContourSize = planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_vecContours.size();
			ROIContourContainer.reserve(ROIContourSize);

			for (unsigned int j = 0; j < ROIContourSize; j++)
			{
				// Define a contour point container
				// Reserve the data buffer
				size_t contourPointsArraySize = planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_vecContours.at(j)->m_arrayPts.size();

				std::vector<PointFloat3D> contourPointsContainer;
				contourPointsContainer.reserve(contourPointsArraySize);

				for (unsigned int k = 0; k < contourPointsArraySize; k++)
				{
					// Get contour point
					PointFloat3D contourPoint;
					contourPoint.x = planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_vecContours.at(j)->m_arrayPts.at(k).fx;
					contourPoint.y = planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_vecContours.at(j)->m_arrayPts.at(k).fy;
					contourPoint.z = planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_vecContours.at(j)->m_arrayPts.at(k).fz;
					contourPoint.strReferencedSOPInstanceUID = (planData->m_pPlannedStructData->m_vecROIItems.at(i)->m_vecContours.at(j)->strReferencedSOPInstanceUID);

					// Push back to contour point container
					contourPointsContainer.push_back(contourPoint);
				}
				// Push back to ROI container
				ROIContourContainer.push_back(contourPointsContainer);
			}
			// Push back to RT struct info container
			RTStructInfoContainer.ROIContour.push_back(ROIContourContainer);
		}

		// Convert it for transverse plane draw data
		RTStructInfoContainer.CTImageAxialContourData.resize(CTInfoContainer.dimension.z);
		RTStructInfoContainer.CTImageAxialContourIndexData.resize(CTInfoContainer.dimension.z);

		for (int s = 0; s < CTInfoContainer.dimension.z; s++)
		{
			for (int i = 0; i < RTStructInfoContainer.ROIContour.size(); i++)
			{
				// Declare container of one region contour data
				std::vector<std::vector<PointFloat3D>> contourDataContainer;
				for (int j = 0; j < RTStructInfoContainer.ROIContour[i].size(); j++)
				{
					// Gathering partial contour data of one region
					std::vector<PointFloat3D> contourDataPoints;
					// Non-coplanar: contour point's z is diffent on a slice, thus contour points on a slice have the same UID
					if (CTInfoContainer.imagePositionPatients[s].strReferencedSOPInstanceUID == RTStructInfoContainer.ROIContour[i][j][0].strReferencedSOPInstanceUID)
					{
						for (int k = 0; k < RTStructInfoContainer.ROIContour[i][j].size(); k++)
						{
							PointFloat3D point = RTStructInfoContainer.ROIContour[i][j][k];
							point.z_sliceIndex = CTInfoContainer.imagePositionPatient.z + s * CTInfoContainer.pixelSpacing.z;
							RTStructInfoContainer.ROIContour[i][j][k].z_sliceIndex = point.z_sliceIndex; // Non-coplanar: Test
							contourDataPoints.push_back(point);
						}
					}

					// If the partial container is not null, add to the region contour data
					if (contourDataPoints.size() > 0)
						contourDataContainer.push_back(contourDataPoints);
				}

				// Add index information if there is contour sequence in current CT image
				if (contourDataContainer.size() > 0)
					RTStructInfoContainer.CTImageAxialContourIndexData[s].push_back(i);

				RTStructInfoContainer.CTImageAxialContourData[s].push_back(contourDataContainer);
			}
		}

		// Set bool data for presentation
		RTStructInfoContainer.ROIPresentation.resize(RTStructInfoContainer.ROIContour.size());
		for (int i = 0; i < RTStructInfoContainer.ROIPresentation.size(); i++)
			RTStructInfoContainer.ROIPresentation[i] = true;

		// Data visualization variable activation
		CTInfoContainer.refreshCTImageTexture = true;
		RTStructInfoContainer.currentCTImageContourDataLoading = true; // Contour data loading
		RTStructInfoContainer.contourDataRemappingForViewRegion = true; // Convert contour point positon to screen position
		originalDataConverted = true;
	}
	else originalDataConverted = true;
}

void IMBTOptimizerDialog_ImGui::calculateDVH(OpenCLHandler* openCLHandler)
{
	/* Single view variables initialization */
	DVHInfoContainer.DVHCalculated = false;

	/* Calculated dose DVH information container initialization */
	DVHInfoContainer.calculatedDVHcGyData.clear();
	DVHInfoContainer.calculatedDVHPercentData.clear();
	DVHInfoContainer.calculatedDVHROIColor.clear();
	DVHInfoContainer.calculatedDVHROIName.clear();

	// Data reconstruction from original data
	std::vector<std::vector<std::vector<CLPosition2D>>> ROISequencePolygon; // ROI -> slice (Z) -> 2D position
	std::vector<std::string> ROISequenceName;
	std::vector<std::string> ROISequenceUID;

	/* DVH calculation */

	// Get geometry information
	// Image Position Patient
	CLPosition3D doseImagePositionPatient;
	doseImagePositionPatient.x = cl_double(CTInfoContainer.imagePositionPatient.x);
	doseImagePositionPatient.y = cl_double(CTInfoContainer.imagePositionPatient.y);
	doseImagePositionPatient.z = cl_double(CTInfoContainer.imagePositionPatient.z);

	// Pixel spacing
	CLPixelSpacing3D dosePixelSpacing;
	dosePixelSpacing.x = cl_double(calculatedDoseContainer.pixelSpacing.x);
	dosePixelSpacing.y = cl_double(calculatedDoseContainer.pixelSpacing.y);
	dosePixelSpacing.z = cl_double(calculatedDoseContainer.pixelSpacing.z);

	// Dose image size
	CLSizeInt3D doseImageSize;
	doseImageSize.x = cl_int(calculatedDoseContainer.dimension.x);
	doseImageSize.y = cl_int(calculatedDoseContainer.dimension.y);
	doseImageSize.z = cl_int(calculatedDoseContainer.dimension.z);

	// Data conversion
	for (int i = 0; i < RTStructInfoContainer.ROIContour.size(); i++)
	{
		std::vector<std::vector<CLPosition2D>> currentContourData;

		for (int j = 0; j < doseImageSize.z; j++) 
		{
			std::vector<CLPosition2D> currentSliceContourData;

			for (int k = 0; k < RTStructInfoContainer.ROIContour[i].size(); k++)
			{
				for (int s = 0; s < RTStructInfoContainer.ROIContour[i][k].size(); s++)
				{
					cl_double contourZPos = RTStructInfoContainer.ROIContour[i][k][s].z;
					cl_double doseSlicePos = doseImagePositionPatient.z + calculatedDoseContainer.pixelSpacing.z * double(j);

					if (abs(contourZPos - doseSlicePos) < 0.1f)
					{
						CLPosition2D currentPositionData{ RTStructInfoContainer.ROIContour[i][k][s].x, RTStructInfoContainer.ROIContour[i][k][s].y };
						currentSliceContourData.push_back(currentPositionData);
					}
				}
			}
			currentContourData.push_back(currentSliceContourData);
		}

		ROISequencePolygon.push_back(currentContourData);
		DVHInfoContainer.calculatedDVHROIColor.push_back(RTStructInfoContainer.ROIColor[i]);
		DVHInfoContainer.calculatedDVHROIName.push_back(RTStructInfoContainer.ROINames[i]);
	}

	// Dose (cGy) information
	std::vector<std::vector<double>> calculatedDoseVoxel;
	for (int i = 0; i < calculatedDoseContainer.dimension.z; i++)
	{
		std::vector<double> doseVoxelSlice;
		for (int j = 0; j < calculatedDoseContainer.dimension.y; j++)
		{
			for (int k = 0; k < calculatedDoseContainer.dimension.x; k++) doseVoxelSlice.push_back(double(calculatedDoseContainer.doseVoxel(i, j, k)));	
		}
		calculatedDoseVoxel.push_back(doseVoxelSlice);
	}

	// Define kernel and queue
	openCLHandler->getOpenCLObject()->kernel = cl::Kernel(openCLHandler->getOpenCLObject()->program, "calDVHDeviceCL");
	openCLHandler->getOpenCLObject()->queue = cl::CommandQueue(openCLHandler->getOpenCLObject()->context, openCLHandler->getOpenCLObject()->device);

	// Input buffer
	cl::Buffer RTDoseImagePositionPatientBuf(openCLHandler->getOpenCLObject()->context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, sizeof(CLPosition3D), &doseImagePositionPatient);
	cl::Buffer RTDosePixelSpacingBuf(openCLHandler->getOpenCLObject()->context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, sizeof(CLPixelSpacing3D), &dosePixelSpacing);
	cl::Buffer RTDoseResolutionBuf(openCLHandler->getOpenCLObject()->context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, sizeof(CLSizeInt3D), &doseImageSize);

	openCLHandler->getOpenCLObject()->kernel.setArg(0, RTDoseImagePositionPatientBuf);
	openCLHandler->getOpenCLObject()->kernel.setArg(1, RTDosePixelSpacingBuf);
	openCLHandler->getOpenCLObject()->kernel.setArg(2, RTDoseResolutionBuf);

	// Create current beam max-dose based dose count vector (10 cGy resolution)
	int doseGyCountVectorSize = int(calculatedDoseContainer.maxDoseValue / 100.f);

	for (int i = 0; i < RTStructInfoContainer.ROIContour.size(); i++)
	{
		dvhCalculationIndex = i;
		int ROIVoxelCount{};

		// Calculate DVH per ROI
		openCLHandler->calDVHHost(&doseImageSize, &ROISequencePolygon[i], &calculatedDoseVoxel, &doseGyCountVectorSize, &DVHInfoContainer.calculatedDVHcGyData, &DVHInfoContainer.calculatedDVHPercentData, &ROIVoxelCount);
		
		// Calculate volume size and save (cm3)
		double ROIVolumeSize = (calculatedDoseContainer.pixelSpacing.x * calculatedDoseContainer.pixelSpacing.y * calculatedDoseContainer.pixelSpacing.z * ROIVoxelCount) / 1000;
		// Save double value
		DVHInfoContainer.calculatedVolumeSize.push_back(ROIVolumeSize);

		// Save string
		int precision = 3; 
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(precision) << ROIVolumeSize;
		std::string ROIVolumeSizeString = ss.str();
		DVHInfoContainer.calculatedVolumeSizeString.push_back(ROIVolumeSizeString);

		// 더 명시적인 방식으로 변경
		//char buffer[64];
		//snprintf(buffer, sizeof(buffer), "%.3f", ROIVolumeSize);
		//std::string ROIVolumeSizeString(buffer);
		//DVHInfoContainer.calculatedVolumeSizeString.push_back(ROIVolumeSizeString);

		double structureMaxDosecGy = *std::max_element(DVHInfoContainer.calculatedDVHcGyData[i].begin(), DVHInfoContainer.calculatedDVHcGyData[i].end());
		if (structureMaxDosecGy > DVHInfoContainer.maxcGyValue) DVHInfoContainer.maxcGyValue = structureMaxDosecGy;
	}

	DVHInfoContainer.calculatingDVH = false;
	DVHInfoContainer.DVHCalculated = true;
}

uint8_t IMBTOptimizerDialog_ImGui::HUtoTextureMapping(int HU, CT &ctInfoContainer)
{
	uint8_t textureValue{};
	float imageMin{ctInfoContainer.m_huCenter - (ctInfoContainer.m_huWidth / 2.0f)};
	float imageMax{ctInfoContainer.m_huCenter + (ctInfoContainer.m_huWidth / 2.0f)};

	if (HU < imageMin)
		textureValue = 0;
	else if (HU > imageMax)
		textureValue = 255;
	else
		textureValue = uint8_t(((HU - imageMin) / ctInfoContainer.m_huWidth) * 255.0f);

	return textureValue;
}

Color IMBTOptimizerDialog_ImGui::getJetColorMap(double v, double vmin, double vmax)
{
	Color c = {1.0, 1.0, 1.0}; // white
	double dv;

	if (v < vmin)
		v = vmin;
	if (v > vmax)
		v = vmax;
	dv = vmax - vmin;

	if (v < (vmin + 0.25 * dv))
	{
		c.R = 0;
		c.G = 4 * (v - vmin) / dv;
	}
	else if (v < (vmin + 0.5 * dv))
	{
		c.R = 0;
		c.B = 1 + 4 * (vmin + 0.25 * dv - v) / dv;
	}
	else if (v < (vmin + 0.75 * dv))
	{
		c.R = 4 * (v - vmin - 0.5 * dv) / dv;
		c.B = 0;
	}
	else
	{
		c.G = 1 + 4 * (vmin + 0.75 * dv - v) / dv;
		c.B = 0;
	}

	return (c);
}

void IMBTOptimizerDialog_ImGui::showMessageBox(std::string sTitle, std::string sText)
{
	ImGui::OpenPopup("MessageBox");

	if (ImGui::BeginPopupModal(sTitle.c_str()))
	{
		ImGui::Text(sText.c_str());

		// Draw popup contents.
		ImGui::EndPopup();
	}
}

bool IMBTOptimizerDialog_ImGui::initializeDoseKernel(CPlan *_plan)
{
	calculatedDoseContainer.InitializedDoseKernel = false;
	calculatedDoseContainer.InitializationInProgress = true;

	m_sOptimizationLog.push_back("[Start Initialization]");

	////////////////////////////////////////////////////////////////////////////////////
	// Arma 3D Matrix for 3D Matrix Processing
	////////////////////////////////////////////////////////////////////////////////////
	std::vector<float> m_posZ;
	std::vector<float> m_posX;
	std::vector<float> m_posY;

	float **ppf = nullptr;
	ppf = _plan->m_pPlannedCTData->m_ppfHU;

	unsigned int rows = _plan->m_pPlannedCTData->m_anDim[0];
	unsigned int cols = _plan->m_pPlannedCTData->m_anDim[1];
	unsigned int SliceSize = _plan->m_pPlannedCTData->m_anDim[2];

	// Arma Cube based CT data structures
	m_pCtMatrix = Cube<float>(rows, cols, SliceSize);

	for (size_t z = 0; z < SliceSize; z++) // Slices
	{
		// arma::Mat<float> tmpSlice(CTs.OriginPixel[z], rows, cols, false, true);
		arma::Mat<float> tmpSlice(ppf[z], rows, cols, false, true);
		m_pCtMatrix.slice(z) = tmpSlice; // for 3d matrix processing
		m_posZ.push_back(_plan->m_pPlannedCTData->m_ptsPatOrg[z].fz);
	}

	// Coordinate of X axis
	for (int i = 0; i < cols; i++)
	{
		m_posX.push_back(CTInfoContainer.imagePositionPatient.x + CTInfoContainer.dimension.x * i);
	}

	// Coordinate of Y axis
	for (int i = 0; i < cols; i++)
	{
		m_posY.push_back(CTInfoContainer.imagePositionPatient.y + CTInfoContainer.dimension.y * i);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// End of Arma 3D Matrix
	////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////
	// Dose Kernel Matrix
	////////////////////////////////////////////////////////////////////////////////////
#ifdef USE_MATLAB_ENGINE
	matlab::data::ArrayFactory factory;

	// Patient ID
	matlab::data::CharArray PatientID = factory.createCharArray(std::string{CT2CA(_plan->m_strID)});

	// Angle Resolution
	// RTPlanContainer.angleResolution = 60;
	// RTPlanContainer.numOfDwellPositionTandem = 27;
	// RTPlanContainer.numOfDwellPositionOvoid = 5;

	matlab::data::Array matlab_AngleResolution = factory.createScalar<float>(RTPlanContainer.angleResolution);
	MatlabEngine::instance().m_pMatlab->setVariable(u"AngleResolution", matlab_AngleResolution);

	// CTData
	auto ct_data = _plan->m_pPlannedCTData;
	matlab::data::StructArray CTData = factory.createStructArray({1, 1}, {"C", "ImagePositionPatient", "ImageOrientationPatient", "PixelSpacing", "SliceThickness", "xPosition", "yPosition", "zPosition"});

	auto *pCtMatrix = &m_pCtMatrix;
	auto C = factory.createArray<double>({pCtMatrix->n_rows, pCtMatrix->n_cols, pCtMatrix->n_slices});
	for (auto slice_index = 0; slice_index < pCtMatrix->n_slices; ++slice_index)
	{
		auto slice = pCtMatrix->slice(slice_index);
		auto row_major_array = factory.createArray({pCtMatrix->n_rows, pCtMatrix->n_cols}, slice.begin(), slice.begin() + slice.n_elem, matlab::data::InputLayout::ROW_MAJOR);
		std::copy_n(row_major_array.begin(), pCtMatrix->slice(slice_index).n_elem, C.begin() + (pCtMatrix->n_rows * pCtMatrix->n_cols * slice_index));
	}
	CTData[0]["C"] = C;

	auto *pImagePositionPatient = (*((CVolumeData *)ct_data)).m_afPatOrg;
	auto ImagePositionPatient = factory.createArray<double>({1, 3});
	std::copy(pImagePositionPatient, pImagePositionPatient + 3, ImagePositionPatient.begin());
	CTData[0]["ImagePositionPatient"] = ImagePositionPatient;

	auto *pImageOrientationPatientHorizontal = (*((CVolumeData *)ct_data)).m_afPatOrientHor;
	auto *pImageOrientationPatientVertical = (*((CVolumeData *)ct_data)).m_afPatOrientVer;
	auto ImageOrientationPatient = factory.createArray<double>({1, 6});
	std::copy(pImageOrientationPatientHorizontal, pImageOrientationPatientHorizontal + 3, ImageOrientationPatient.begin());
	std::copy(pImageOrientationPatientVertical, pImageOrientationPatientVertical + 3, ImageOrientationPatient.begin() + 3);
	CTData[0]["ImageOrientationPatient"] = ImageOrientationPatient;

	auto *pPixelSpacing = (*((CVolumeData *)ct_data)).m_afRes;
	auto PixelSpacing = factory.createArray<double>({1, 2});
	std::copy(pPixelSpacing, pPixelSpacing + 2, PixelSpacing.begin());
	CTData[0]["PixelSpacing"] = PixelSpacing;

	auto *pSliceThickness = (*((CVolumeData *)ct_data)).m_afRes + 2;
	auto SliceThickness = factory.createArray<double>({1, 1});
	std::copy(pSliceThickness, pSliceThickness + 1, SliceThickness.begin());
	CTData[0]["SliceThickness"] = SliceThickness;

	CTData[0]["xPosition"] = factory.createArray({1, m_posX.size()}, m_posX.begin(), m_posX.end());
	CTData[0]["yPosition"] = factory.createArray({1, m_posY.size()}, m_posY.begin(), m_posY.end());
	CTData[0]["zPosition"] = factory.createArray({1, m_posZ.size()}, m_posZ.begin(), m_posZ.end());

	// StructData
	auto struct_data = _plan->m_pPlannedStructData;
	matlab::data::StructArray StructData = factory.createStructArray({1, 1}, {"structname", "x", "y", "z"});

	std::vector<std::string> structnames;
	std::vector<std::vector<std::vector<float>>> x_list;
	std::vector<std::vector<std::vector<float>>> y_list;
	std::vector<std::vector<std::vector<float>>> z_list;

	for (auto &item : struct_data->m_vecROIItems)
	{
		structnames.push_back(std::string{CT2CA(item->m_strName)});
		std::vector<std::vector<float>> item_x_list;
		std::vector<std::vector<float>> item_y_list;
		std::vector<std::vector<float>> item_z_list;
		for (auto &contour : item->m_vecContours)
		{
			std::vector<float> contour_x_list;
			std::vector<float> contour_y_list;
			std::vector<float> contour_z_list;
			for (auto arrayPt : contour->m_arrayPts)
			{
				contour_x_list.push_back(arrayPt.fx);
				contour_y_list.push_back(arrayPt.fy);
				contour_z_list.push_back(arrayPt.fz);
			}
			item_x_list.push_back(contour_x_list);
			item_y_list.push_back(contour_y_list);
			item_z_list.push_back(contour_z_list);
		}
		x_list.push_back(item_x_list);
		y_list.push_back(item_y_list);
		z_list.push_back(item_z_list);
	}

	auto structname = factory.createArray({1, structnames.size()}, structnames.begin(), structnames.end());
	StructData[0]["structname"] = structname;
	StructData[0]["x"] = _createCellArray(x_list);
	StructData[0]["y"] = _createCellArray(y_list);
	StructData[0]["z"] = _createCellArray(z_list);

	// Catheter
	auto catheter_data = _plan->m_vecCatheters;
	matlab::data::StructArray Catheter = factory.createStructArray({1, 1}, {"ch1", "ch2", "ch3"});

	std::vector<std::array<float, 3>> ch1;
	for (auto position : catheter_data[0]->m_vecDwellPositions)
	{
		ch1.push_back(std::array<float, 3>{position.m_fptPosition.fx, position.m_fptPosition.fy, position.m_fptPosition.fz});
	}
	std::vector<std::array<float, 3>> ch2;
	for (auto position : catheter_data[1]->m_vecDwellPositions)
	{
		ch2.push_back(std::array<float, 3>{position.m_fptPosition.fx, position.m_fptPosition.fy, position.m_fptPosition.fz});
	}
	std::vector<std::array<float, 3>> ch3;
	for (auto position : catheter_data[2]->m_vecDwellPositions)
	{
		ch3.push_back(std::array<float, 3>{position.m_fptPosition.fx, position.m_fptPosition.fy, position.m_fptPosition.fz});
	}

	auto ch1_array = factory.createArray<double>({ch1.size(), 3});
	for (auto i = 0; i < ch1.size(); ++i)
	{
		ch1_array[i][0] = (double)ch1[i][0];
		ch1_array[i][1] = (double)ch1[i][1];
		ch1_array[i][2] = (double)ch1[i][2];
	}
	Catheter[0]["ch1"] = ch1_array;

	auto ch2_array = factory.createArray<double>({ch2.size(), 3});
	for (auto i = 0; i < ch2.size(); ++i)
	{
		ch2_array[i][0] = (double)ch2[i][0];
		ch2_array[i][1] = (double)ch2[i][1];
		ch2_array[i][2] = (double)ch2[i][2];
	}
	Catheter[0]["ch2"] = ch2_array;

	auto ch3_array = factory.createArray<double>({ch3.size(), 3});
	for (auto i = 0; i < ch3.size(); ++i)
	{
		ch3_array[i][0] = (double)ch3[i][0];
		ch3_array[i][1] = (double)ch3[i][1];
		ch3_array[i][2] = (double)ch3[i][2];
	}
	Catheter[0]["ch3"] = ch3_array;

	std::vector<matlab::data::Array> args({PatientID, CTData, StructData, Catheter});
#else
	std::string PatientID{CT2CA(_plan->m_strID)};
	auto AngleResolution = RTPlanContainer.angleResolution;

	std::vector<double> ImageOrientationPatient;
	ImageOrientationPatient.insert(ImageOrientationPatient.end(), &_plan->m_pPlannedCTData->m_afPatOrientHor[0], &_plan->m_pPlannedCTData->m_afPatOrientHor[0] + 3);
	ImageOrientationPatient.insert(ImageOrientationPatient.end(), &_plan->m_pPlannedCTData->m_afPatOrientVer[0], &_plan->m_pPlannedCTData->m_afPatOrientVer[0] + 3);

	std::vector<double> PixelSpacing;
	PixelSpacing.insert(PixelSpacing.end(), &_plan->m_pPlannedCTData->m_afRes[0], &_plan->m_pPlannedCTData->m_afRes[0] + 2);

	json CTData;
	CTData["C"] = m_pCtMatrix;
	CTData["ImagePositionPatient"] = _plan->m_pPlannedCTData->m_afPatOrg;
	CTData["ImageOrientationPatient"] = ImageOrientationPatient;
	CTData["PixelSpacing"] = PixelSpacing;
	CTData["SliceThickness"] = _plan->m_pPlannedCTData->m_afRes[2];
	CTData["xPosition"] = m_posX;
	CTData["yPosition"] = m_posY;
	CTData["zPosition"] = m_posZ;

	std::vector<std::string> structname;
	std::vector<std::vector<std::vector<float>>> x;
	std::vector<std::vector<std::vector<float>>> y;
	std::vector<std::vector<std::vector<float>>> z;
	for (auto &item : _plan->m_pPlannedStructData->m_vecROIItems)
	{
		structname.push_back(std::string{CT2CA(item->m_strName)});

		std::vector<std::vector<float>> item_x_list;
		std::vector<std::vector<float>> item_y_list;
		std::vector<std::vector<float>> item_z_list;
		for (auto &contour : item->m_vecContours)
		{
			std::vector<float> contour_x_list;
			std::vector<float> contour_y_list;
			std::vector<float> contour_z_list;
			for (auto arrayPt : contour->m_arrayPts)
			{
				contour_x_list.push_back(arrayPt.fx);
				contour_y_list.push_back(arrayPt.fy);
				contour_z_list.push_back(arrayPt.fz);
			}
			item_x_list.push_back(contour_x_list);
			item_y_list.push_back(contour_y_list);
			item_z_list.push_back(contour_z_list);
		}
		x.push_back(item_x_list);
		y.push_back(item_y_list);
		z.push_back(item_z_list);
	}

	json StructData;
	StructData["structname"] = structname;
	StructData["x"] = x;
	StructData["y"] = y;
	StructData["z"] = z;

	std::vector<std::array<float, 3>> ch1;
	for (auto position : _plan->m_vecCatheters[0]->m_vecDwellPositions)
	{
		ch1.push_back(std::array<float, 3>{position.m_fptPosition.fx, position.m_fptPosition.fy, position.m_fptPosition.fz});
	}
	std::vector<std::array<float, 3>> ch2;
	for (auto position : _plan->m_vecCatheters[1]->m_vecDwellPositions)
	{
		ch2.push_back(std::array<float, 3>{position.m_fptPosition.fx, position.m_fptPosition.fy, position.m_fptPosition.fz});
	}
	std::vector<std::array<float, 3>> ch3;
	for (auto position : _plan->m_vecCatheters[2]->m_vecDwellPositions)
	{
		ch3.push_back(std::array<float, 3>{position.m_fptPosition.fx, position.m_fptPosition.fy, position.m_fptPosition.fz});
	}

	json Catheter;
	Catheter["ch1"] = ch1;
	Catheter["ch2"] = ch2;
	Catheter["ch3"] = ch3;
#endif

	// Check output
	ofstream writeOutput("matlab_output_dose_kernel.txt");

	try
	{
		typedef std::basic_stringbuf<char16_t> StringBuf;
		std::shared_ptr<StringBuf> output = std::make_shared<StringBuf>();
		std::shared_ptr<StringBuf> error = std::make_shared<StringBuf>();

		if (writeOutput.is_open())
		{
			writeOutput << "############# Creating Dose Kernel" << std::endl;
			m_sOptimizationLog.push_back("# Creating Dose Kernel");

#ifdef USE_MATLAB_ENGINE
			MatlabEngine::instance().m_pMatlab->setVariable(u"PatientID", PatientID);
			MatlabEngine::instance().m_pMatlab->setVariable(u"CTData", CTData);
			MatlabEngine::instance().m_pMatlab->setVariable(u"StructData", StructData);
			MatlabEngine::instance().m_pMatlab->setVariable(u"Catheter", Catheter);

			const size_t numReturned = 0;
			auto future = MatlabEngine::instance().m_pMatlab->evalAsync(u"[PTV, bladder, rectum, bowel, sigmoid, body, ind_bodyd] = create_dosematrix_pure_matlab_c__(PatientID, CTData, StructData, Catheter, AngleResolution);", output, error);
			future.wait();

			writeOutput << matlab::engine::convertUTF16StringToUTF8String(output.get()->str()).c_str();
			writeOutput << std::endl;
#else
			create_dosematrix_pure_matlab_c__(PatientID, CTData, StructData, Catheter, AngleResolution);
#endif

			m_sOptimizationLog.push_back("# Created Dose Kernel");
		}
	}
	catch (const std::exception e)
	{
		writeOutput << e.what();
		return false;
	}

	writeOutput.close();

	calculatedDoseContainer.InitializedDoseKernel = true;
	calculatedDoseContainer.InitializationInProgress = false;

	return true;
}

#ifdef USE_MATLAB_ENGINE
bool IMBTOptimizerDialog_ImGui::optimizeIMBT_Matlab(CPlan *_pPlan)
{
	m_sOptimizationLog.push_back("# Optimization: Started");
	calculatedDoseContainer.calculationComplete = false;
	calculatedDoseContainer.calculationInProcess = true;

	if (calculatedDoseContainer.InitializedDoseKernel == false)
	{
		showMessageBox("No initialized", "Warning");

		return false;
	}

	ofstream writeOutput("matlab_output_optimization.txt");

	try
	{
#ifdef USE_MATLAB_ENGINE
		matlab::data::ArrayFactory factory;

		typedef std::basic_stringbuf<char16_t> StringBuf;
		std::shared_ptr<StringBuf> output = std::make_shared<StringBuf>();
		std::shared_ptr<StringBuf> error = std::make_shared<StringBuf>();
		const size_t numReturned = 0;

		// Prescription Dose (cGy) and Normalization
		matlab::data::Array prescriptionDose = factory.createScalar<float>(RTPlanContainer.prescriptionDose);
		matlab::data::Array NormalizationOfPTV = factory.createScalar<float>(RTPlanContainer.normalization);

		MatlabEngine::instance().m_pMatlab->setVariable(u"PrescriptionDose", prescriptionDose);
		MatlabEngine::instance().m_pMatlab->setVariable(u"Normalization", NormalizationOfPTV);

		auto future = MatlabEngine::instance().m_pMatlab->evalAsync(u"[final_dose_512, dwellPositions]=main_Brachy_GS_L0_patient2_ovoid_2022_matlab_c__(PrescriptionDose, Normalization, AngleResolution, PTV, bladder, rectum, bowel, sigmoid, body, ind_bodyd);", output, error);
		future.wait();

		// future = MatlabEngine::instance().m_pMatlab->evalAsync(u"implay(final_dose_512);", output, error);
		// future.wait();

		writeOutput << matlab::engine::convertUTF16StringToUTF8String(output.get()->str()).c_str();

		// Import Dose Matrix
		future = MatlabEngine::instance().m_pMatlab->evalAsync(u"single(final_dose_512);", output, error);
		future.wait();

		matlab::data::TypedArray<double> dd = MatlabEngine::instance().m_pMatlab->getVariable(u"final_dose_512"); // cGy
		matlab::data::ArrayDimensions matlab_doseDimension = dd.getDimensions();

		unsigned int dataSize = matlab_doseDimension[0] * matlab_doseDimension[1];

		// Set dimension of calculated voxel, it is same with CT voxel in this time
		xt::xarray<size_t>::shape_type doseDimension = {size_t(matlab_doseDimension[2]), size_t(matlab_doseDimension[0]), size_t(matlab_doseDimension[1])};
		calculatedDoseContainer.dimension.z = doseDimension.at(0);
		calculatedDoseContainer.dimension.x = doseDimension.at(1);
		calculatedDoseContainer.dimension.y = doseDimension.at(2);

		// Set pixel spacing of calculated voxel
		calculatedDoseContainer.pixelSpacing.x = _pPlan->m_pPlannedCTData->m_afRes[0];
		calculatedDoseContainer.pixelSpacing.y = _pPlan->m_pPlannedCTData->m_afRes[1];
		calculatedDoseContainer.pixelSpacing.z = _pPlan->m_pPlannedCTData->m_afRes[2];

		calculatedDoseContainer.doseScalingFactor = 1.0f;

		std::vector<float> doseVolumeVector;
		float fMaxDoseValue = -1.0f;
		for (int i = 0; i < matlab_doseDimension[2]; i++) // slice
		{
			for (int ny = 0; ny < matlab_doseDimension[0]; ny++) // rows
			{
				for (int nx = 0; nx < matlab_doseDimension[1]; nx++) // cols
				{
					float fValue = (float)dd[ny][nx][i];

					if (fValue < 0.f)
					{
						fValue = 0.f;
					}

					float scaledValue = fValue * calculatedDoseContainer.doseScalingFactor;

					if (scaledValue > fMaxDoseValue)
						fMaxDoseValue = scaledValue;

					doseVolumeVector.push_back(scaledValue);
				}
			}
		}

		calculatedDoseContainer.maxDoseValue = fMaxDoseValue;
		calculatedDoseContainer.doseVoxel = xt::adapt(doseVolumeVector, doseDimension);
#else
		// extern std::tuple<cube, vec> main_Brachy_GS_L0_patient2_ovoid_2022_matlab_c__(double PrescriptionDose, double Normalization, int AngleResolution,
		//	mat PTV, mat bladder, mat rectum, mat bowel, mat sigmoid, mat body, uvec ind_bodyd);

		double PrescriptionDose{RTPlanContainer.prescriptionDose};
		double Normalization{RTPlanContainer.normalization};
		int AngleResolution{RTPlanContainer.angleResolution};
		mat PTV;
		mat bladder;
		mat rectum;
		mat bowel;
		mat sigmoid;
		mat body;
		uvec ind_bodyd;

		main_Brachy_GS_L0_patient2_ovoid_2022_matlab_c__(
			PrescriptionDose,
			Normalization,
			AngleResolution,
			PTV,
			bladder,
			rectum,
			bowel,
			sigmoid,
			body,
			ind_bodyd);
#endif
	}
	catch (const std::exception e)
	{
		m_sOptimizationLog.push_back("[Error Occurred] " + std::string(e.what()));
		return false;
	}

	writeOutput.close();

	calculatedDoseContainer.calculationComplete = true;
	calculatedDoseContainer.calculationInProcess = false;
	calculatedDoseContainer.refreshDoseTexture = true;
	DVHInfoContainer.calculatingDVH = false;
	DVHInfoContainer.DVHCalculated = false;

	m_sOptimizationLog.push_back("# Optimization: Completed");

	return true;
}
#else
bool IMBTOptimizerDialog_ImGui::optimizeIMBT_Matlab(CPlan *_pPlan)
{
	m_sOptimizationLog.push_back("# Optimization: Started");
	calculatedDoseContainer.calculationComplete = false;
	calculatedDoseContainer.calculationInProcess = true;

	if (!calculatedDoseContainer.InitializedDoseKernel)
	{
		showMessageBox("No initialized", "Warning");
		m_sOptimizationLog.push_back("[Error] Dose Kernel Not Initialized");
		calculatedDoseContainer.calculationInProcess = false;
		return false;
	}

	try
	{
		// 실제 최적화/선량 계산 (C++로 대체)
		// 1) 처방선량, 정규화 등 가져오기
		double PrescriptionDose = RTPlanContainer.prescriptionDose;
		double Normalization = RTPlanContainer.normalization;
		int AngleResolution = RTPlanContainer.angleResolution;

		// Call optimization function with member variables
		auto [doseCube, dwellPos] = main_Brachy_GS_L0_patient2_ovoid_2022_matlab_c__(
			PrescriptionDose,
			Normalization,
			AngleResolution,
			m_PTV,
			m_bladder,
			m_rectum,
			m_bowel,
			m_sigmoid,
			m_body,
			m_ind_bodyd);

		// Set calculated dose container properties
		calculatedDoseContainer.dimension = {
			static_cast<size_t>(doseCube.n_slices),
			static_cast<size_t>(doseCube.n_rows),
			static_cast<size_t>(doseCube.n_cols)};

		calculatedDoseContainer.pixelSpacing = {
			_pPlan->m_pPlannedCTData->m_afRes[0],
			_pPlan->m_pPlannedCTData->m_afRes[1],
			_pPlan->m_pPlannedCTData->m_afRes[2]};

		calculatedDoseContainer.doseScalingFactor = 1.0f;

		// Convert armadillo cube to vector
		std::vector<float> doseVolumeVector;
		doseVolumeVector.reserve(doseCube.n_elem);
		float maxDose = -1.0f;

		for (size_t i = 0; i < doseCube.n_slices; ++i)
		{
			for (size_t j = 0; j < doseCube.n_rows; ++j)
			{
				for (size_t k = 0; k < doseCube.n_cols; ++k)
				{
					float value = (float)doseCube(j, k, i);
					value = std::max(0.0f, value);
					value *= calculatedDoseContainer.doseScalingFactor;
					maxDose = std::max(maxDose, value);
					doseVolumeVector.push_back(value);
				}
			}
		}

		calculatedDoseContainer.maxDoseValue = maxDose;

		// Create xtensor shape and adapt vector to it
		xt::xarray<size_t>::shape_type shape = {
			calculatedDoseContainer.dimension.z,
			calculatedDoseContainer.dimension.x,
			calculatedDoseContainer.dimension.y};
		calculatedDoseContainer.doseVoxel = xt::adapt(doseVolumeVector, shape);

		// Store dwell positions
		calculatedDoseContainer.dwellPositions.clear();
		calculatedDoseContainer.dwellPositions.reserve(dwellPos.n_elem);
		for (size_t i = 0; i < dwellPos.n_elem; ++i)
		{
			calculatedDoseContainer.dwellPositions.push_back(dwellPos(i));
		}

		calculatedDoseContainer.calculationComplete = true;
		m_sOptimizationLog.push_back("# Optimization: Completed");
	}
	catch (const std::exception &e)
	{
		m_sOptimizationLog.push_back("[Error Occurred] " + std::string(e.what()));
		calculatedDoseContainer.calculationComplete = false;
		return false;
	}

	calculatedDoseContainer.calculationInProcess = false;
	return true;
}
#endif

#ifdef USE_MATLAB_ENGINE
void IMBTOptimizerDialog_ImGui::ConfirmOptimization(CPlan *_pPlan)
{
	_pPlan->m_pCalculatedDose->m_anDim[0] = calculatedDoseContainer.dimension.y; // rows
	_pPlan->m_pCalculatedDose->m_anDim[1] = calculatedDoseContainer.dimension.x; // cols
	_pPlan->m_pCalculatedDose->m_anDim[2] = calculatedDoseContainer.dimension.z; // Slices

	_pPlan->m_pCalculatedDose->m_afPatOrg[0] = _pPlan->m_pPlannedCTData->m_afPatOrg[0];
	_pPlan->m_pCalculatedDose->m_afPatOrg[1] = _pPlan->m_pPlannedCTData->m_afPatOrg[1];
	_pPlan->m_pCalculatedDose->m_afPatOrg[2] = _pPlan->m_pPlannedCTData->m_afPatOrg[2];

	_pPlan->m_pCalculatedDose->m_afPatOrientHor[0] = _pPlan->m_pPlannedCTData->m_afPatOrientHor[0];
	_pPlan->m_pCalculatedDose->m_afPatOrientHor[1] = _pPlan->m_pPlannedCTData->m_afPatOrientHor[1];
	_pPlan->m_pCalculatedDose->m_afPatOrientHor[2] = _pPlan->m_pPlannedCTData->m_afPatOrientHor[2];

	_pPlan->m_pCalculatedDose->m_afPatOrientVer[0] = _pPlan->m_pPlannedCTData->m_afPatOrientVer[0];
	_pPlan->m_pCalculatedDose->m_afPatOrientVer[1] = _pPlan->m_pPlannedCTData->m_afPatOrientVer[1];
	_pPlan->m_pCalculatedDose->m_afPatOrientVer[2] = _pPlan->m_pPlannedCTData->m_afPatOrientVer[2];

	_pPlan->m_pCalculatedDose->m_afRes[0] = _pPlan->m_pPlannedCTData->m_afRes[0];
	_pPlan->m_pCalculatedDose->m_afRes[1] = _pPlan->m_pPlannedCTData->m_afRes[1];
	_pPlan->m_pCalculatedDose->m_afRes[2] = _pPlan->m_pPlannedCTData->m_afRes[2];

	_pPlan->m_pCalculatedDose->m_fDoseGridScaling = calculatedDoseContainer.doseScalingFactor;

	_pPlan->m_pCalculatedDose->m_ppwDen = new float *[calculatedDoseContainer.dimension.z];

	int dataSize = calculatedDoseContainer.dimension.x * calculatedDoseContainer.dimension.y;
	for (int i = 0; i < calculatedDoseContainer.dimension.z; i++)
	{
		_pPlan->m_pCalculatedDose->m_ppwDen[i] = new float[dataSize];
	}
	std::vector<float> dose;
	for (int i = 0; i < calculatedDoseContainer.dimension.z; i++) // slice
	{
		for (int ny = 0; ny < calculatedDoseContainer.dimension.y; ny++) // rows
		{
			for (int nx = 0; nx < calculatedDoseContainer.dimension.x; nx++) // cols
			{
				_pPlan->m_pCalculatedDose->m_ppwDen[i][ny * calculatedDoseContainer.dimension.y + nx] = calculatedDoseContainer.doseVoxel(i, ny, nx);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// Dwell Position and Time
	//////////////////////////////////////////////////////////////////////////////////////////
	matlab::data::Array dwellPositionsFromMatlab = MatlabEngine::instance().m_pMatlab->getVariable(u"dwellPositions");

	if (_pPlan->m_vecCatheters.size() >= 3) // Ovoid
	{
		// dwellPositions: Angle1 pos1 pos2 pos3 ... posN, Angle2 pos1 pos2 pos3 ... PosN ...
		// RTPlanContainer.angleResolution;
		// RTPlanContainer.numOfDwellPositionOvoid;
		// RTPlanContainer.numOfDwellPositionTandem;
		// Start Angle = -180.0 to +180.0
		int numOfAngles = 360 / RTPlanContainer.angleResolution;
		int tandem_Range = numOfAngles * RTPlanContainer.numOfDwellPositionTandem;
		int ovoidL_Range = tandem_Range + 5;
		int ovoidR_Range = ovoidL_Range + 5;

		// Initialize
		for (int i = 0; i < _pPlan->m_vecCatheters.size(); i++)
		{
			for (int j = 0; j < _pPlan->m_vecCatheters[i]->m_vecDwellPositions.size(); j++)
			{
				_pPlan->m_vecCatheters[i]->m_vecDwellPositions[j].m_bIsActive = false;
				_pPlan->m_vecCatheters[i]->m_vecDwellPositions[j].m_nAngle = 0;
				_pPlan->m_vecCatheters[i]->m_vecDwellPositions[j].m_vecAnglDwell.clear();
			}
		}

		for (int i = 0; i < dwellPositionsFromMatlab.getNumberOfElements(); i++)
		{
			double dwellTime = double(dwellPositionsFromMatlab[i]) / RTPlanContainer.fractions;

			if (i < tandem_Range)
			{

				int dwellPositionIndex = i % RTPlanContainer.numOfDwellPositionTandem;

				if (dwellTime > 0)
				{
					AngleDwellTime tmp;
					tmp.m_fDwellTime = dwellTime;
					tmp.m_fAngle = (-180) + int(i / RTPlanContainer.numOfDwellPositionTandem) * RTPlanContainer.angleResolution;

					_pPlan->m_vecCatheters[2]->m_vecDwellPositions[dwellPositionIndex].m_bIsActive = true;
					_pPlan->m_vecCatheters[2]->m_vecDwellPositions[dwellPositionIndex].m_nAngle += 1;

					_pPlan->m_vecCatheters[2]->m_vecDwellPositions[dwellPositionIndex].m_vecAnglDwell.push_back(tmp);
				}
			}
			else if (i >= tandem_Range && i < ovoidL_Range)
			{
				AngleDwellTime tmp;
				int dwellPositionIndex = (i - tandem_Range);

				tmp.m_fAngle = 0;
				tmp.m_fDwellTime = dwellTime;

				if (dwellTime)
					_pPlan->m_vecCatheters[1]->m_vecDwellPositions[dwellPositionIndex].m_bIsActive = true;
				else
					_pPlan->m_vecCatheters[1]->m_vecDwellPositions[dwellPositionIndex].m_bIsActive = false;

				_pPlan->m_vecCatheters[1]->m_vecDwellPositions[dwellPositionIndex].m_vecAnglDwell.push_back(tmp);
			}
			else
			{
				AngleDwellTime tmp;
				int dwellPositionIndex = (i - ovoidL_Range);

				tmp.m_fAngle = 0;
				tmp.m_fDwellTime = dwellTime;

				if (dwellTime)
					_pPlan->m_vecCatheters[0]->m_vecDwellPositions[dwellPositionIndex].m_bIsActive = true;
				else
					_pPlan->m_vecCatheters[0]->m_vecDwellPositions[dwellPositionIndex].m_bIsActive = false;

				_pPlan->m_vecCatheters[0]->m_vecDwellPositions[dwellPositionIndex].m_vecAnglDwell.push_back(tmp);
			}
		}
	}

	MatlabEngine::instance().stopMatlabEngine();

	calculatedDoseContainer.calculatedDoseConfirm = true;
	*m_CalculatedDoseConfirm = true;  // 외부 변수에 명시적 저장

	m_pDialog->closeDialog();
}
#else
void IMBTOptimizerDialog_ImGui::ConfirmOptimization(CPlan *_pPlan)
{
	// Dose dimension ����
	_pPlan->m_pCalculatedDose->m_anDim[0] = calculatedDoseContainer.dimension.y; // rows
	_pPlan->m_pCalculatedDose->m_anDim[1] = calculatedDoseContainer.dimension.x; // cols
	_pPlan->m_pCalculatedDose->m_anDim[2] = calculatedDoseContainer.dimension.z; // Slices

	// CT �����ͷκ��� patient orientation ����
	for (int i = 0; i < 3; ++i)
	{
		_pPlan->m_pCalculatedDose->m_afPatOrg[i] = _pPlan->m_pPlannedCTData->m_afPatOrg[i];
		_pPlan->m_pCalculatedDose->m_afPatOrientHor[i] = _pPlan->m_pPlannedCTData->m_afPatOrientHor[i];
		_pPlan->m_pCalculatedDose->m_afPatOrientVer[i] = _pPlan->m_pPlannedCTData->m_afPatOrientVer[i];
		_pPlan->m_pCalculatedDose->m_afRes[i] = _pPlan->m_pPlannedCTData->m_afRes[i];
	}

	_pPlan->m_pCalculatedDose->m_fDoseGridScaling = calculatedDoseContainer.doseScalingFactor;

	// Dose matrix �޸� �Ҵ�
	const size_t dataSize = calculatedDoseContainer.dimension.x * calculatedDoseContainer.dimension.y;
	_pPlan->m_pCalculatedDose->m_ppwDen = new float *[calculatedDoseContainer.dimension.z];

	for (size_t i = 0; i < calculatedDoseContainer.dimension.z; i++)
	{
		_pPlan->m_pCalculatedDose->m_ppwDen[i] = new float[dataSize];
	}

	// Dose data ����
	for (size_t i = 0; i < calculatedDoseContainer.dimension.z; i++)
	{
		for (size_t ny = 0; ny < calculatedDoseContainer.dimension.y; ny++)
		{
			for (size_t nx = 0; nx < calculatedDoseContainer.dimension.x; nx++)
			{
				_pPlan->m_pCalculatedDose->m_ppwDen[i][ny * calculatedDoseContainer.dimension.y + nx] =
					calculatedDoseContainer.doseVoxel(i, ny, nx);
			}
		}
	}

	// Dwell Position �� Time ó��
	if (_pPlan->m_vecCatheters.size() >= 3)
	{ // Ovoid case
		const int numOfAngles = 360 / RTPlanContainer.angleResolution;
		const int tandem_Range = numOfAngles * RTPlanContainer.numOfDwellPositionTandem;
		const int ovoidL_Range = tandem_Range + 5;
		const int ovoidR_Range = ovoidL_Range + 5;

		// Catheter dwell positions �ʱ�ȭ
		for (auto &catheter : _pPlan->m_vecCatheters)
		{
			for (auto &dwellPos : catheter->m_vecDwellPositions)
			{
				dwellPos.m_bIsActive = false;
				dwellPos.m_nAngle = 0;
				dwellPos.m_vecAnglDwell.clear();
			}
		}

		// Dwell positions ó��
		const auto &dwellPositions = calculatedDoseContainer.dwellPositions;

		for (size_t i = 0; i < dwellPositions.size(); i++)
		{
			double dwellTime = dwellPositions[i] / RTPlanContainer.fractions;

			if (i < tandem_Range)
			{
				// Tandem ó��
				int dwellPositionIndex = i % RTPlanContainer.numOfDwellPositionTandem;

				if (dwellTime > 0)
				{
					AngleDwellTime tmp;
					tmp.m_fDwellTime = dwellTime;
					tmp.m_fAngle = (-180) + int(i / RTPlanContainer.numOfDwellPositionTandem) * RTPlanContainer.angleResolution;

					auto &dwellPos = _pPlan->m_vecCatheters[2]->m_vecDwellPositions[dwellPositionIndex];
					dwellPos.m_bIsActive = true;
					dwellPos.m_nAngle += 1;
					dwellPos.m_vecAnglDwell.push_back(tmp);
				}
			}
			else if (i < ovoidL_Range)
			{
				// Left Ovoid ó��
				int dwellPositionIndex = (i - tandem_Range);
				AngleDwellTime tmp{0, dwellTime};

				auto &dwellPos = _pPlan->m_vecCatheters[1]->m_vecDwellPositions[dwellPositionIndex];
				dwellPos.m_bIsActive = (dwellTime > 0);
				dwellPos.m_vecAnglDwell.push_back(tmp);
			}
			else
			{
				// Right Ovoid ó��
				int dwellPositionIndex = (i - ovoidL_Range);
				AngleDwellTime tmp{0, dwellTime};

				auto &dwellPos = _pPlan->m_vecCatheters[0]->m_vecDwellPositions[dwellPositionIndex];
				dwellPos.m_bIsActive = (dwellTime > 0);
				dwellPos.m_vecAnglDwell.push_back(tmp);
			}
		}
	}
	// OpenCL 의존성 제거 - DVH 계산이 필요하지 않음
	calculatedDoseContainer.calculatedDoseConfirm = true;
	*m_CalculatedDoseConfirm = true;
	m_pDialog->closeDialog();
}
#endif