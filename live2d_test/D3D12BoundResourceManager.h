#pragma once

#include "D3D12Manager.h"
#include "d3dcompiler.h"
#include <unordered_map>
#include <array>

namespace D3D
{
    class D3D12BoundResourceManager
    {
    public:
        enum DefaultSamplerType { kPointWrap, kPointClamp, kLinearWrap, kLinearClamp, kAnisotropicWrap, kAnisotropicClamp };

        D3D12BoundResourceManager();
        ~D3D12BoundResourceManager();

        void Initialize(ID3DBlob *const shader_arr[5]);

        D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(const std::string &res_name, uint32_t index);
        bool BindDefaultSampler(const std::string& sampler_name, uint32_t index, DefaultSamplerType default_sampler);

        const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputElemDescArray();
        ID3D12RootSignature* GetRootSignature();

    private:
        struct TmpBindPointData
        {
            uint32_t max_bind_point = 0;
            uint32_t min_bind_point = 0x10000;
        };

        struct DescriptorRangeBindPointDesc
        {
            D3D12_DESCRIPTOR_RANGE_TYPE range_type = {};
            uint32_t bind_point = 0;
            uint32_t bind_count = 0;
            uint32_t space = 0;
            uint32_t root_signature_offset = 0;
        };

        struct ResourceBindInfo
        {
            D3D12_SHADER_INPUT_BIND_DESC bind_desc = {};
            DescriptorRangeBindPointDesc* descriptor_range_desc = nullptr;
        };

        using ShaderInputBindMap = std::unordered_map<std::string, ResourceBindInfo>;
        using RangeBindPointDescArray = std::array<DescriptorRangeBindPointDesc, 4>;

        std::vector<D3D12_SIGNATURE_PARAMETER_DESC>         input_paramters_;
        std::vector<D3D12_INPUT_ELEMENT_DESC>               input_elements_;
        ShaderInputBindMap                                  resource_bind_map_;
        RangeBindPointDescArray                             bound_point_map_;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        srv_uav_cbv_heap_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        sampler_heap_;
        Microsoft::WRL::ComPtr<ID3D12RootSignature>         root_signature_;

        static DXGI_FORMAT GetDxgiFormatFromSemanticName(const std::string& semnatic_name);
        static std::string GetShaderResourceIdentify(const std::string& raw_name);
        static D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangeType(D3D_SHADER_INPUT_TYPE shader_input_type);
        static void UpdateResourceBoundPoint(TmpBindPointData& resource_bound_desc, DescriptorRangeBindPointDesc& shader_bind_desc, const D3D12_SHADER_INPUT_BIND_DESC& shader_input_desc);
        static std::vector<D3D12_DESCRIPTOR_RANGE> GenerateDescriptorRange(RangeBindPointDescArray& range_bind_array);
        static const char* SwitchSemanticName(const std::string& semantic_name);
        static D3D12_SAMPLER_DESC GetDefaultSamplerDesc(DefaultSamplerType default_sampler);

        std::vector<D3D12_INPUT_ELEMENT_DESC> ParserVsInputParamters(const std::vector<D3D12_SIGNATURE_PARAMETER_DESC>& input_paramters);
        void InitializeBoundResource(ID3DBlob *const shader_arr[5]);
        void InitializeDescriptorHeap();
        void InitializeRootSignature();
    };

};
