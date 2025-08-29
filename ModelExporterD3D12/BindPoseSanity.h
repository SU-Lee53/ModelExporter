#pragma once
#include <assimp/scene.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <cstdio>

using namespace DirectX;

struct BindPoseReport {
    struct Row {
        std::string boneName;
        float err_parentLocal_GlobalMul_TransposeNo = NAN;
        float err_parentLocal_GlobalMul_TransposeYes = NAN;
        float err_localParent_GlobalMul_TransposeNo = NAN;
        float err_localParent_GlobalMul_TransposeYes = NAN;
        float err_parentLocal_OffsetFirst_TransposeNo = NAN;
        float err_parentLocal_OffsetFirst_TransposeYes = NAN;
        // ...�ʿ��ϸ� �� �߰�
    };
    std::vector<Row> rows;
    float maxErr = 0.0f;
    std::string bestCombo; // ��ü������ ���� �� ���� ������ ����
};

// --- Assimp �� XMMATRIX ��ȯ(��ġ ���� ����) ---
inline XMMATRIX AiToXM(const aiMatrix4x4& A, bool transpose) {
    // Assimp�� ��/�� �Ծ� ȥ���� ���. ��ġ �ɼ����� ��� �˻��غ���.
    XMMATRIX M = XMMATRIX(
        A.a1, A.a2, A.a3, A.a4,
        A.b1, A.b2, A.b3, A.b4,
        A.c1, A.c2, A.c3, A.c4,
        A.d1, A.d2, A.d3, A.d4
    );
    return transpose ? XMMatrixTranspose(M) : M;
}

// --- Frobenius norm of (M - I) ---
inline float IdentityError(const XMMATRIX& M) {
    XMFLOAT4X4 m; XMStoreFloat4x4(&m, M);
    float s = 0.0f;
    const float I[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    const float* p = &m.m[0][0];
    for (int i = 0; i < 16; i++) {
        float d = p[i] - I[i];
        s += d * d;
    }
    return sqrtf(s);
}

// --- ���� �� ������ ---
inline void BuildNodeMap(aiNode* n, std::unordered_map<std::string, aiNode*>& out) {
    out[n->mName.C_Str()] = n;
    for (unsigned i = 0; i < n->mNumChildren; i++) BuildNodeMap(n->mChildren[i], out);
}

// --- bone �̸� �� Offset, bone �̸� �� Node ���� ���� ---
struct BoneData {
    std::unordered_map<std::string, XMMATRIX> offsetByName_T0; // transpose=false
    std::unordered_map<std::string, XMMATRIX> offsetByName_T1; // transpose=true
    std::unordered_map<std::string, aiNode*>  nodeByName;
};

inline BoneData CollectBones(const aiScene* scn) {
    BoneData bd;
    BuildNodeMap(scn->mRootNode, bd.nodeByName);
    for (unsigned mi = 0; mi < scn->mNumMeshes; ++mi) {
        const aiMesh* m = scn->mMeshes[mi];
        for (unsigned bi = 0; bi < m->mNumBones; ++bi) {
            const aiBone* b = m->mBones[bi];
            std::string name = b->mName.C_Str();
            // ���� mesh�� �ߺ� ���� ���� �� ó�� �͸� ���ų� ���ϼ� ����
            if (!bd.offsetByName_T0.count(name)) {
                bd.offsetByName_T0[name] = AiToXM(b->mOffsetMatrix, /*transpose=*/false);
                bd.offsetByName_T1[name] = AiToXM(b->mOffsetMatrix, /*transpose=*/true);
            }
        }
    }
    return bd;
}

// --- GlobalBind ��� (parent*local �Ǵ� local*parent ��� ����) ---
enum class MulOrder { ParentTimesLocal, LocalTimesParent };

inline void DFS_GlobalBind(
    aiNode* node,
    const XMMATRIX& parent,
    std::unordered_map<std::string, XMMATRIX>& outGlobal,
    bool transposeLocal,
    MulOrder order
) {
    XMMATRIX L = AiToXM(node->mTransformation, transposeLocal);
    XMMATRIX G = (order == MulOrder::ParentTimesLocal) ? XMMatrixMultiply(parent, L)
        : XMMatrixMultiply(L, parent);
    outGlobal[node->mName.C_Str()] = G;
    for (unsigned i = 0; i < node->mNumChildren; i++)
        DFS_GlobalBind(node->mChildren[i], G, outGlobal, transposeLocal, order);
}

// --- ���� ��Ŀ��� ���� ---
inline BindPoseReport TestBindPose(const aiScene* scn) {
    BindPoseReport rep;
    BoneData bd = CollectBones(scn);

    // 4���� ������ �׽�Ʈ:
    //  (1) Global = parent*local,  Offset�� transpose ����/��
    //  (2) Global = local*parent,  Offset�� transpose ����/��
    //  �Դٰ� final ���� ����( Global*Offset vs Offset*Global )�� �� �� �׽�Ʈ
    std::unordered_map<std::string, XMMATRIX> G_PxL_T0, G_PxL_T1, G_LxP_T0, G_LxP_T1;
    DFS_GlobalBind(scn->mRootNode, XMMatrixIdentity(), G_PxL_T0, /*transposeLocal=*/false, MulOrder::ParentTimesLocal);
    DFS_GlobalBind(scn->mRootNode, XMMatrixIdentity(), G_PxL_T1, /*transposeLocal=*/true, MulOrder::ParentTimesLocal);
    DFS_GlobalBind(scn->mRootNode, XMMatrixIdentity(), G_LxP_T0, /*transposeLocal=*/false, MulOrder::LocalTimesParent);
    DFS_GlobalBind(scn->mRootNode, XMMatrixIdentity(), G_LxP_T1, /*transposeLocal=*/true, MulOrder::LocalTimesParent);

    float bestMaxErr = std::numeric_limits<float>::infinity();
    std::string bestCombo;

    for (auto& [name, node] : bd.nodeByName) {
        if (!bd.offsetByName_T0.count(name)) continue; // ���� �ƴ� �Ϲ� ���� �ǳʶ�
        BindPoseReport::Row row; row.boneName = name;

        auto eval = [&](const XMMATRIX& G, const XMMATRIX& O, bool offsetFirst)->float {
            // row-vector ������ ����: v' = v * M �� �Ϲ������� final = Global * Offset
            XMMATRIX E = offsetFirst ? XMMatrixMultiply(O, G) : XMMatrixMultiply(G, O);
            return IdentityError(E);
            };

        const XMMATRIX& O0 = bd.offsetByName_T0[name];
        const XMMATRIX& O1 = bd.offsetByName_T1[name];

        row.err_parentLocal_GlobalMul_TransposeNo = eval(G_PxL_T0[name], O0, /*offsetFirst=*/false);
        row.err_parentLocal_GlobalMul_TransposeYes = eval(G_PxL_T1[name], O1, /*offsetFirst=*/false);
        row.err_localParent_GlobalMul_TransposeNo = eval(G_LxP_T0[name], O0, /*offsetFirst=*/false);
        row.err_localParent_GlobalMul_TransposeYes = eval(G_LxP_T1[name], O1, /*offsetFirst=*/false);
        row.err_parentLocal_OffsetFirst_TransposeNo = eval(G_PxL_T0[name], O0, /*offsetFirst=*/true);
        row.err_parentLocal_OffsetFirst_TransposeYes = eval(G_PxL_T1[name], O1, /*offsetFirst=*/true);

        rep.rows.push_back(row);
    }

    // � ������ ������ �� �´����� ã��
    auto accumulateMax = [&](auto proj)->float {
        float mx = 0.0f;
        for (auto& r : rep.rows) mx = std::max(mx, proj(r));
        return mx;
        };
    struct Opt { const char* name; float mx; };
    std::vector<Opt> opts = {
        { "Global=Parent*Local, final=Global*Offset, transpose=No",  accumulateMax([](auto& r) {return r.err_parentLocal_GlobalMul_TransposeNo; }) },
        { "Global=Parent*Local, final=Global*Offset, transpose=Yes", accumulateMax([](auto& r) {return r.err_parentLocal_GlobalMul_TransposeYes; }) },
        { "Global=Local*Parent, final=Global*Offset, transpose=No",  accumulateMax([](auto& r) {return r.err_localParent_GlobalMul_TransposeNo; }) },
        { "Global=Local*Parent, final=Global*Offset, transpose=Yes", accumulateMax([](auto& r) {return r.err_localParent_GlobalMul_TransposeYes; }) },
        { "Global=Parent*Local, final=Offset*Global, transpose=No",  accumulateMax([](auto& r) {return r.err_parentLocal_OffsetFirst_TransposeNo; }) },
        { "Global=Parent*Local, final=Offset*Global, transpose=Yes", accumulateMax([](auto& r) {return r.err_parentLocal_OffsetFirst_TransposeYes; }) },
    };

    float mx = std::numeric_limits<float>::infinity();
    std::string pick;
    for (auto& o : opts) if (o.mx < mx) { mx = o.mx; pick = o.name; }

    rep.maxErr = mx;
    rep.bestCombo = pick;
    return rep;
}

// --- �ܼ�/�α� ��� ����� ---
inline void PrintReport(const BindPoseReport& rep) {
    std::printf("==== BindPose Sanity Report ====\n");
    std::printf("Best Global/Offset convention: %s\n", rep.bestCombo.c_str());
    std::printf("Max error (Frobenius): %.6g\n", rep.maxErr);
    for (auto& r : rep.rows) {
        std::printf("Bone %-32s | PxL G*O T0: %.3e | PxL G*O T1: %.3e | LxP G*O T0: %.3e | PxL O*G T0: %.3e\n",
            r.boneName.c_str(),
            r.err_parentLocal_GlobalMul_TransposeNo,
            r.err_parentLocal_GlobalMul_TransposeYes,
            r.err_localParent_GlobalMul_TransposeNo,
            r.err_parentLocal_OffsetFirst_TransposeNo
        );
    }
    std::printf("================================\n");
}

inline void ShowBindPoseReportImGui(const BindPoseReport& rep, bool* p_open = nullptr)
{
    if (!ImGui::Begin("Bind Pose Sanity Report", p_open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    // ��� ���
    ImGui::TextUnformatted("Best convention:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.55f, 1.0f), "%s", rep.bestCombo.c_str());
    ImGui::Text("Max error (Frobenius): %.6g", rep.maxErr);

    // ����/�Ӱ谪
    static ImGuiTextFilter filter;
    filter.Draw("Bone filter");
    ImGui::SameLine();

    static float threshold = 1e-4f;     // �Ӱ� ���ϸ� �������� ����
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputFloat("error threshold", &threshold, 0.0f, 0.0f, "%.1e");

    ImGui::Separator();

    // ���̺� �÷���
    ImGuiTableFlags flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable;

    // ���̺�: Row ������ 6�� ��ǥ�� ���� ����
    const float table_height = ImGui::GetTextLineHeightWithSpacing() * 22.0f;
    if (ImGui::BeginTable("BindPoseSanityTable", 7, flags, ImVec2(0, table_height)))
    {
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableSetupColumn("Bone", ImGuiTableColumnFlags_WidthFixed, 240.0f);
        ImGui::TableSetupColumn("PxL  G*O  T0"); // err_parentLocal_GlobalMul_TransposeNo
        ImGui::TableSetupColumn("PxL  G*O  T1"); // err_parentLocal_GlobalMul_TransposeYes
        ImGui::TableSetupColumn("LxP  G*O  T0"); // err_localParent_GlobalMul_TransposeNo
        ImGui::TableSetupColumn("LxP  G*O  T1"); // err_localParent_GlobalMul_TransposeYes
        ImGui::TableSetupColumn("PxL  O*G  T0"); // err_parentLocal_OffsetFirst_TransposeNo
        ImGui::TableSetupColumn("PxL  O*G  T1"); // err_parentLocal_OffsetFirst_TransposeYes
        ImGui::TableHeadersRow();

        auto CellErr = [&](float v)
            {
                if (std::isnan(v)) { ImGui::TextUnformatted("-"); return; }
                if (v > threshold)
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%.3e", v); // �Ӱ� �ʰ�
                else
                    ImGui::Text("%.3e", v);
            };

        for (const auto& r : rep.rows)
        {
            if (!filter.PassFilter(r.boneName.c_str())) continue;

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(r.boneName.c_str());

            ImGui::TableSetColumnIndex(1);
            CellErr(r.err_parentLocal_GlobalMul_TransposeNo);

            ImGui::TableSetColumnIndex(2);
            CellErr(r.err_parentLocal_GlobalMul_TransposeYes);

            ImGui::TableSetColumnIndex(3);
            CellErr(r.err_localParent_GlobalMul_TransposeNo);

            ImGui::TableSetColumnIndex(4);
            CellErr(r.err_localParent_GlobalMul_TransposeYes);

            ImGui::TableSetColumnIndex(5);
            CellErr(r.err_parentLocal_OffsetFirst_TransposeNo);

            ImGui::TableSetColumnIndex(6);
            CellErr(r.err_parentLocal_OffsetFirst_TransposeYes);
        }

        ImGui::EndTable();
    }

    // ������
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Legend:");
    ImGui::BulletText("PxL = Global = Parent * Local");
    ImGui::BulletText("LxP = Global = Local * Parent");
    ImGui::BulletText("G*O = E = Global * Offset   (Expects Identity)");
    ImGui::BulletText("O*G = E = Offset * Global   (For Convention Chect)");
    ImGui::BulletText("T0/T1 = Assimp -> DirectX (Transpose) NO/YES");

    ImGui::End();
}
static BindPoseReport g_Report;
static bool g_HasReport = false;

void AfterSceneLoaded(const aiScene* scene)
{
    g_Report = TestBindPose(scene); // ���� �Լ� �״�� ���
    g_HasReport = true;
}

// �� ������(Imgui ������ ��)
void DrawDebugUI()
{
    if (g_HasReport) {
        static bool open = true;
        ShowBindPoseReportImGui(g_Report, &open);
    }
}
