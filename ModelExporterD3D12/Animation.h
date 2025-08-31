#pragma once

constexpr UINT MAX_BONE_COUNT = 250;

/*
	08.23 갈아엎는다
		- AnimationNode 는 Bone 으로 대체
		- IMPORT 시에는 Bone 정보와 keyframe 변환 정보를 std::map으로 묶어서 받아옴
		- 모든 애니메이션 정보와 최종 행렬 + 이를 GPU로 보낼 ID3D12Resource 는 Root 의 Animation 객체에만 보관
			- 자식 GameObject 들의 Animation 은 nullptr
		- 나머지 자식 노드들은 Bone 정보를 GPU에 업로드하여 애니메이션 재생

		- 현재 흐른 시간에 맞게 keyframe 들을 보간하여 Bone 들의 행렬 array 를 만들어 GPU로 전송 -> StructuredBuffer?
		- 결국 GPU가 갖는 값은:
			[Bone #0 Key Matrix][Bone #1 Key Matrix][Bone #2 Key Matrix] ... [Bone #250 Key Matrix] -> 최대 250개의 Bone 을 지원
		- 나중에 자식들을 렌더링할시 Bone 별로 알맞는 인덱스를 찾아가도록 Bone 정보를 GPU로 보내주어야 함
		- 애니메이션 행렬들은 Bone Local 변환이므로 GameObject::xmf4x4Transforms 처럼 부모 누적이 필요함
			- 이 누적을 어떻게 할것인가가 현재 최대 문제점인거 같음
		- 할꺼 많노

	08.24
		- AnimationNode 가 필요하긴 할듯 -> 이전처럼 각 자식이 아닌 Root 의 Animation 객체가 가지고있도록 하고 GameObject 처럼 계층 구조를 갖도록 한다
*/


struct AnimKeyframe {
	float fTime;
	XMFLOAT3 xmf3PositionKey;
	XMFLOAT4 xmf4RotationKey;
	XMFLOAT3 xmf3ScaleKey;
};

struct AnimChannel {
	std::string strName;	// Bone Name
	std::vector<AnimKeyframe> keyframes;
};

struct ANIMATION_IMPORT_INFO {
	std::string strAnimationName;
	float fDuration = 0.f;
	float fTicksPerSecond = 0.f;
	float fFrameRate = 0.f;

	std::vector<AnimChannel> animationDatas;
};

struct CB_ANIMATION_CONTROL_DATA {
	int		nCurrentAnimationIndex;
	float	fDuration;
	float	fTicksPerSecond;
	float	fTimeElapsed;
};

struct CB_ANIMATION_TRANSFORM_DATA {
	XMFLOAT4X4 xmf4x4Transforms[MAX_BONE_COUNT];
};

class Animation : public std::enable_shared_from_this<Animation> {
public:
    struct Clip {
        std::string name;
        float       durationTicks = 0.f;
        float       ticksPerSecond = 0.f;
        float       frameRate = 30.f;
        unsigned    frameCount = 0;
        std::unordered_map<std::string, std::vector<AnimKeyframe>> channels; // node/bone name -> frames

        XMFLOAT4X4 GetSRT(const std::string& strBoneKey, float fTime, BOOL bSRTOrder) {
            auto it = channels.find(strBoneKey);
            if (it == channels.end()) {
                XMFLOAT4X4 xmf4x4Ret;
                XMStoreFloat4x4(&xmf4x4Ret, XMMatrixIdentity());
                return xmf4x4Ret;
            }

            const std::vector<AnimKeyframe>& keyframes = it->second;
            XMFLOAT4X4 xmf4x4SRT;

            if (keyframes.size() == 1) {
                auto keyframe = keyframes[0];
                XMVECTOR S = XMLoadFloat3(&keyframe.xmf3ScaleKey);
                XMVECTOR R = XMLoadFloat4(&keyframe.xmf4RotationKey);
                XMVECTOR T = XMLoadFloat3(&keyframe.xmf3PositionKey);

                XMMATRIX Sm = XMMatrixScalingFromVector(S);
                XMMATRIX Rm = XMMatrixRotationQuaternion(XMQuaternionNormalize(R));
                XMMATRIX Tm = XMMatrixTranslationFromVector(T);

                XMMATRIX xmmtxSRT;

                if (bSRTOrder) {
                    xmmtxSRT = XMMatrixMultiply(Sm, XMMatrixMultiply(Rm, Tm));
                }
                else {
                    xmmtxSRT = XMMatrixMultiply(Tm, XMMatrixMultiply(Rm, Sm));
                }

                XMStoreFloat4x4(&xmf4x4SRT, xmmtxSRT);
                return xmf4x4SRT;
            }

            for (int i = 0; i < keyframes.size() - 1; ++i) {
                if ((fTime >= keyframes[i].fTime) && (fTime <= keyframes[i + 1].fTime)) {
                    float fInterval = keyframes[i + 1].fTime - keyframes[i].fTime;
                    float t = (fTime - keyframes[i].fTime) / fInterval;
                    XMVECTOR S0 = XMLoadFloat3(&keyframes[i].xmf3ScaleKey);
                    XMVECTOR S1 = XMLoadFloat3(&keyframes[i + 1].xmf3ScaleKey);
                    XMVECTOR R0 = XMLoadFloat4(&keyframes[i].xmf4RotationKey);
                    XMVECTOR R1 = XMLoadFloat4(&keyframes[i + 1].xmf4RotationKey);
                    XMVECTOR T0 = XMLoadFloat3(&keyframes[i].xmf3PositionKey);
                    XMVECTOR T1 = XMLoadFloat3(&keyframes[i + 1].xmf3PositionKey);

                    XMVECTOR S = XMVectorLerp(S0, S1, t);
                    XMVECTOR R = XMQuaternionSlerp(R0, R1, t);
                    XMVECTOR T = XMVectorLerp(T0, T1, t);
                    //XMMATRIX xmmtxSRT = XMMatrixAffineTransformation(S, XMVectorSet(0.f, 0.f, 0.f, 1.f), R, T)

                    XMMATRIX Sm = XMMatrixScalingFromVector(S);
                    XMMATRIX Rm = XMMatrixRotationQuaternion(XMQuaternionNormalize(R));
                    XMMATRIX Tm = XMMatrixTranslationFromVector(T);

                    XMMATRIX xmmtxSRT;

                    if (bSRTOrder) {
                        xmmtxSRT = XMMatrixMultiply(Sm, XMMatrixMultiply(Rm, Tm));
                    }
                    else {
                        xmmtxSRT = XMMatrixMultiply(Tm, XMMatrixMultiply(Rm, Sm));
                    }


                    XMStoreFloat4x4(&xmf4x4SRT, xmmtxSRT);
                    return xmf4x4SRT;
                }
            }

            if ((fTime >= keyframes.back().fTime)) {
                float fInterval = keyframes.front().fTime - keyframes.back().fTime;
                float t = (fTime - keyframes.back().fTime) / fInterval;
                XMVECTOR S0 = XMLoadFloat3(&keyframes.back().xmf3ScaleKey);
                XMVECTOR S1 = XMLoadFloat3(&keyframes.front().xmf3ScaleKey);
                XMVECTOR R0 = XMLoadFloat4(&keyframes.back().xmf4RotationKey);
                XMVECTOR R1 = XMLoadFloat4(&keyframes.front().xmf4RotationKey);
                XMVECTOR T0 = XMLoadFloat3(&keyframes.back().xmf3PositionKey);
                XMVECTOR T1 = XMLoadFloat3(&keyframes.front().xmf3PositionKey);

                XMVECTOR S = XMVectorLerp(S0, S1, t);
                XMVECTOR R = XMQuaternionSlerp(R0, R1, t);
                XMVECTOR T = XMVectorLerp(T0, T1, t);
                //XMMATRIX xmmtxSRT = XMMatrixAffineTransformation(S, XMVectorSet(0.f, 0.f, 0.f, 1.f), R, T)

                XMMATRIX Sm = XMMatrixScalingFromVector(S);
                XMMATRIX Rm = XMMatrixRotationQuaternion(XMQuaternionNormalize(R));
                XMMATRIX Tm = XMMatrixTranslationFromVector(T);

                XMMATRIX xmmtxSRT;

                if (bSRTOrder) {
                    xmmtxSRT = XMMatrixMultiply(Sm, XMMatrixMultiply(Rm, Tm));
                }
                else {
                    xmmtxSRT = XMMatrixMultiply(Tm, XMMatrixMultiply(Rm, Sm));
                }


                XMStoreFloat4x4(&xmf4x4SRT, xmmtxSRT);
                return xmf4x4SRT;

            }


            XMStoreFloat4x4(&xmf4x4SRT, XMMatrixIdentity());
            return xmf4x4SRT;
        }

    };

public:
    static std::shared_ptr<Animation>
        LoadFromInfo(Microsoft::WRL::ComPtr<ID3D12Device> device,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd,
            const std::vector<ANIMATION_IMPORT_INFO>& infos,
            std::shared_ptr<GameObject> pRoot);

    void UpdateShaderVariables(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd);

    void Play(bool on) { m_bPlay = on; }
    void SetClip(int idx);
    int  GetClip() const { return m_nCurrentAnimationIndex; }
    float GetTime() const { return m_fTimeElapsed; }
    void  SetTime(float t) { m_fTimeElapsed = t; }
    void  SetOwner(std::shared_ptr<GameObject> owner) { m_wpOwner = owner; }

    // 애니메이션이 “없을 때”도 VS가 b5를 요구하면 바인딩해야 함 → 아이덴티티 본 CB 제공
    static void BindIdentityBones(Microsoft::WRL::ComPtr<ID3D12Device> device,
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd,
        UINT rootParamIndexForBones);

    // 루트에서만 호출 (둘 다 CBV 바인딩)
    void BindCBVs(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmd,
        UINT rootParamForCtrl, UINT rootParamForBones);

    auto& GetBones() { return m_bones; }

public:
    static constexpr UINT ANIMATION_DESCRIPTOR_COUNT_PER_DRAW = 0;

private:
    Animation() = default;

    void buildBoneOrderFromImporterOrder(); // OBJECT_IMPORT_INFO::boneMap 기준
    void allocateCBuffers(Microsoft::WRL::ComPtr<ID3D12Device> device);

    void buildLocalMapForFrame(std::unordered_map<std::string, DirectX::XMMATRIX>& outLocal);
    void buildGlobalByDFS(std::shared_ptr<GameObject> node,
        const std::unordered_map<std::string, DirectX::XMMATRIX>& locals,
        std::unordered_map<std::string, DirectX::XMMATRIX>& outGlobal,
        DirectX::XMMATRIX parentGlobal);
public:
    // 현재 클립의 시간 m_fTimeElapsed 기준으로 글로벌 행렬 맵을 만들어준다.
    // key = 노드명, value = Global(t)
    void BuildGlobalForDebug(std::unordered_map<std::string, DirectX::XMMATRIX>& outGlobal) const;

private:
    std::weak_ptr<GameObject> m_wpOwner;

    std::vector<Clip>         m_Clips;
    int                       m_nCurrentAnimationIndex = 0;
    float                     m_fTimeElapsed = 0.f;
    bool                      m_bPlay = false;

    std::vector<Bone>      m_bones;

    ComPtr<ID3D12Resource>    m_pControllerCBuffer = nullptr;
    UINT8* m_pControllerDataMappedPtr = nullptr;

    ComPtr<ID3D12Resource>    m_pBoneTransformCBuffer = nullptr;
    UINT8* m_pBoneTransformMappedPtr = nullptr;

    // 아이덴티티 본 CB (애니 없음/스킨 없음 대응)
    static ComPtr<ID3D12Resource> s_pIdentityBonesCB;
    static UINT8* s_pIdentityBonesMapped;
    static bool                   s_identityReady;

private:
    // Debug
    BOOL m_bGlobalMulOrder = true;
    BOOL m_bFinalTranspose = true;
    BOOL m_bLocalSRT = true;


};