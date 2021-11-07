#include "OVR.h"
#include "Debug.h"
#include "Extras/OVR_Math.h"

using namespace OVR;

Wolf::OVR::OVR(VkDevice device, VkCommandPool commandPool, Queue graphicsQueue, ovrSession session, ovrGraphicsLuid luid)
{	
	m_session = session;
	m_luid = luid;

	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
	m_size = ovr_GetFovTextureSize(m_session, ovrEye_Left, hmdDesc.DefaultEyeFov[ovrEye_Left], 1);

	{
		ovrErrorInfo errorInfo;
		ovr_GetLastErrorInfo(&errorInfo);
		int a = 0;
	}

	char extensionNames[4096];
	uint32_t extensionNamesSize = sizeof(extensionNames);;
	auto ret = ovr_GetDeviceExtensionsVk(luid, extensionNames, &extensionNamesSize);

	{
		/*ovrTextureSwapChainDesc depthDesc = {};
		depthDesc.Type = ovrTexture_2D;
		depthDesc.ArraySize = 1;
		depthDesc.Format = OVR_FORMAT_D32_FLOAT;
		depthDesc.Width = size.w;
		depthDesc.Height = size.h;
		depthDesc.MipLevels = 1;
		depthDesc.SampleCount = 1;
		depthDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
		depthDesc.BindFlags = ovrTextureBind_DX_DepthStencil;
		depthDesc.StaticImage = ovrFalse;
		ovr_CreateTextureSwapChainVk(session, device, &depthDesc, &m_depthChain);*/

		ovrTextureSwapChainDesc colorDesc = {};
		colorDesc.Type = ovrTexture_2D;
		colorDesc.ArraySize = 1;
		colorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		colorDesc.Width = m_size.w;
		colorDesc.Height = m_size.h;
		colorDesc.MipLevels = 1;
		colorDesc.SampleCount = 1;
		colorDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
		colorDesc.BindFlags = ovrTextureBind_DX_RenderTarget | ovrTextureBind_DX_UnorderedAccess;
		colorDesc.StaticImage = ovrFalse;
		ovrResult result = ovr_CreateTextureSwapChainVk(m_session, device, &colorDesc, &m_textureChain);
		if (!OVR_SUCCESS(result))
		{
			ovrErrorInfo errorInfo;
			ovr_GetLastErrorInfo(&errorInfo);
			Debug::sendError("Failed to create swapchain OVR");
			return;
		}
	}

	{
		int textureCount = 0;
		ovrResult result = ovr_GetTextureSwapChainLength(m_session, m_textureChain, &textureCount);
		if (!OVR_SUCCESS(result))
		{
			ovrErrorInfo errorInfo;
			ovr_GetLastErrorInfo(&errorInfo);
			Debug::sendError("Failed to create swapchain OVR");
			return;
		}

		m_swapchainImages.reserve(textureCount);
		for (int i = 0; i < textureCount; ++i)
		{
			VkImage colorImage;
			ovr_GetTextureSwapChainBufferVk(m_session, m_textureChain, i, &colorImage);

			m_swapchainImages.emplace_back(Image(device, commandPool, graphicsQueue, colorImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, { (uint32_t)m_size.w, (uint32_t)m_size.h }));
		}
	}

	double frameTiming = ovr_GetPredictedDisplayTime(m_session, m_frameIndex);
}

Wolf::OVR::~OVR()
{
}

void Wolf::OVR::update()
{
	ovrResult result = ovr_WaitToBeginFrame(m_session, m_frameIndex);
	result = ovr_BeginFrame(m_session, m_frameIndex);
	
	ovrEyeRenderDesc eyeRenderDesc[2];
	ovrPosef      hmdToEyeViewPose[2];
	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(m_session);
	eyeRenderDesc[0] = ovr_GetRenderDesc(m_session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(m_session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
	hmdToEyeViewPose[0] = eyeRenderDesc[0].HmdToEyePose;
	hmdToEyeViewPose[1] = eyeRenderDesc[1].HmdToEyePose;

	m_layer.Header.Type = ovrLayerType_EyeFov;
	m_layer.Header.Flags = 0;
	m_layer.ColorTexture[0] = m_textureChain;
	//m_layer.DepthTexture[0] = m_depthChainLeft;

	m_layer.ColorTexture[1] = m_textureChain;
	//m_layer.DepthTexture[1] = m_depthChainRight;

	/*m_layer.Fov[0] = eyeRenderDesc[0].Fov;
	m_layer.Fov[1] = eyeRenderDesc[1].Fov;*/

	m_layer.Viewport[0] = { 0, 0, m_size.w / 2, m_size.h };
	m_layer.Viewport[1] = { m_size.w / 2, 0, m_size.w / 2, m_size.h };

	ovrPosef eyeRenderPose[ovrEye_Count];
	ovrPosef HmdToEyePose[ovrEye_Count] = { eyeRenderDesc[ovrEye_Left].HmdToEyePose,
											  eyeRenderDesc[ovrEye_Right].HmdToEyePose };
	double sensorSampleTime;

	ovr_GetEyePoses(m_session, m_frameIndex, ovrTrue, HmdToEyePose, eyeRenderPose, &sensorSampleTime);
	m_layer.RenderPose[0] = eyeRenderPose[0];
	m_layer.RenderPose[1] = eyeRenderPose[1];

	m_layer.Fov[0] = hmdDesc.DefaultEyeFov[0];
	m_layer.Fov[1] = hmdDesc.DefaultEyeFov[1];

	m_layer.SensorSampleTime = sensorSampleTime;

	static float yaw(0.0f);
	Matrix4f rollPitchYaw = Matrix4f::RotationY(yaw);
	Vector3f playerPos(m_playerPos.x, m_playerPos.y, m_playerPos.z);

	// Left
	{
		// Get view and projection matrices
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(eyeRenderPose[0].Orientation);
		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
		Vector3f shiftedEyePos = playerPos + rollPitchYaw.Transform(eyeRenderPose[0].Position);

		Matrix4f view = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
		Matrix4f proj = Matrix4f::Scaling(1.0f, -1.0f, 1.0f) * ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[0], 0.2f, 100.0f, ovrProjection_None);

		glm::mat4 glmView(view.M[0][0], view.M[0][1], view.M[0][2], view.M[0][3],
			view.M[1][0], view.M[1][1], view.M[1][2], view.M[1][3],
			view.M[2][0], view.M[2][1], view.M[2][2], view.M[2][3],
			view.M[3][0], view.M[3][1], view.M[3][2], view.M[3][3]);
		m_viewMatrices[0] = glm::transpose(glmView);

		glm::mat4 glmProjection(proj.M[0][0], proj.M[0][1], proj.M[0][2], proj.M[0][3],
			proj.M[1][0], proj.M[1][1], proj.M[1][2], proj.M[1][3],
			proj.M[2][0], proj.M[2][1], proj.M[2][2], proj.M[2][3],
			proj.M[3][0], proj.M[3][1], proj.M[3][2], proj.M[3][3]);
		m_projMatrices[0] = glm::transpose(glmProjection);

		m_eyeDirections[0].x = finalForward.x;
		m_eyeDirections[0].y = finalForward.y;
		m_eyeDirections[0].z = finalForward.z;
	}

	// Right
	{
		// Get view and projection matrices
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(eyeRenderPose[1].Orientation);
		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
		Vector3f shiftedEyePos = playerPos + rollPitchYaw.Transform(eyeRenderPose[1].Position);

		Matrix4f view = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
		Matrix4f proj = Matrix4f::Scaling(1.0f, -1.0f, 1.0f) * ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[1], 0.2f, 100.0f, ovrProjection_None);

		glm::mat4 glmView(view.M[0][0], view.M[0][1], view.M[0][2], view.M[0][3],
			view.M[1][0], view.M[1][1], view.M[1][2], view.M[1][3],
			view.M[2][0], view.M[2][1], view.M[2][2], view.M[2][3],
			view.M[3][0], view.M[3][1], view.M[3][2], view.M[3][3]);
		m_viewMatrices[1] = glm::transpose(glmView);

		glm::mat4 glmProjection(proj.M[0][0], proj.M[0][1], proj.M[0][2], proj.M[0][3],
			proj.M[1][0], proj.M[1][1], proj.M[1][2], proj.M[1][3],
			proj.M[2][0], proj.M[2][1], proj.M[2][2], proj.M[2][3],
			proj.M[3][0], proj.M[3][1], proj.M[3][2], proj.M[3][3]);
		m_projMatrices[1] = glm::transpose(glmProjection);

		m_eyeDirections[1].x = finalForward.x;
		m_eyeDirections[1].y = finalForward.y;
		m_eyeDirections[1].z = finalForward.z;
	}

	for (int eye = 0; eye < 2; ++eye)
	{
		m_eyePositions[eye].x = playerPos.x + eyeRenderPose[eye].Position.x;
		m_eyePositions[eye].y = playerPos.y + eyeRenderPose[eye].Position.y;
		m_eyePositions[eye].z = playerPos.z + eyeRenderPose[eye].Position.z;
	}
}

int Wolf::OVR::getCurrentImage(VkDevice device, VkQueue presentQueue)
{
	ovr_SetSynchronizationQueueVk(m_session, presentQueue);
	
	int index = 0;
	ovr_GetTextureSwapChainCurrentIndex(m_session, m_textureChain, &index);

	return index;
}

void Wolf::OVR::present(int imageIndex)
{
	ovrLayerHeader* layers = &m_layer.Header;

	ovrResult result = ovr_CommitTextureSwapChain(m_session, m_textureChain);

	result = ovr_EndFrame(m_session, m_frameIndex, nullptr, &layers, 1);

	m_frameIndex++;
}

std::vector<Wolf::Image*> Wolf::OVR::getImages()
{
	std::vector<Image*> r;
	for (size_t i(0); i < m_swapchainImages.size(); ++i)
		r.push_back(&m_swapchainImages[i]);

	return r;
}
