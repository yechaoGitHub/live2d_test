#include "D3D12BoundResourceManager.h"
#include "D3DUtil.h"

#include "DirectXTK/DescriptorHeap.h"

#include <regex>

namespace D3D
{
    using namespace Microsoft::WRL;
    using namespace DirectX;

    const std::vector<std::string> SEMANTIC_NAMES = { "POSITION", "NORMAL", "TANGENT", "TEXCOORD" };

    D3D12BoundResourceManager::D3D12BoundResourceManager()
    {
    }

    D3D12BoundResourceManager::~D3D12BoundResourceManager()
    {
    }

    void D3D12BoundResourceManager::Initialize(ID3DBlob *const shader_arr[5])
    {
        InitializeBoundResource(shader_arr);
        InitializeDescriptorHeap();
        InitializeRootSignature();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12BoundResourceManager::GetDescriptorHandle(const std::string& res_name, uint32_t index)
    {
        auto find_it = resource_bind_map_.find(res_name);
        if (find_it == resource_bind_map_.end())
        {
            return {};
        }

        auto& res_bind = find_it->second;
        auto& descriptor_range_desc = res_bind.descriptor_range_desc;

        if (index >= descriptor_range_desc->bind_count)
        {
            return {};
        }

        switch (descriptor_range_desc->range_type)
        {
            case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
            case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
            case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
            {
                DescriptorHeap dx_descriptor_heap(srv_uav_cbv_heap_.Get());
                return dx_descriptor_heap.GetCpuHandle(descriptor_range_desc->root_signature_offset + res_bind.bind_desc.BindPoint + index);
            }
            break;

            case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
            {
                DescriptorHeap dx_descriptor_heap(sampler_heap_.Get());
                return dx_descriptor_heap.GetCpuHandle(descriptor_range_desc->root_signature_offset + res_bind.bind_desc.BindPoint + index);
            }
            break;

            default:
                ThrowIfFailed(0);
            break;
        }

        ThrowIfFailed(0);
        return {};
    }

    bool D3D12BoundResourceManager::BindDefaultSampler(const std::string& sampler_name, uint32_t index, DefaultSamplerType default_sampler)
    {
        auto cpu_descriptpr = GetDescriptorHandle(sampler_name, index);
        if (cpu_descriptpr.ptr == 0)
        {
            return false;
        }

        auto&& desc = GetDefaultSamplerDesc(default_sampler);
        D3D12Manager::GetDevice()->CreateSampler(&desc, cpu_descriptpr);
        return true;
    }

    const std::vector<D3D12_INPUT_ELEMENT_DESC>& D3D12BoundResourceManager::GetInputElemDescArray()
    {
        return input_elements_;
    }

    ID3D12RootSignature* D3D12BoundResourceManager::GetRootSignature()
    {
        return root_signature_.Get();
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> D3D12BoundResourceManager::ParserVsInputParamters(const std::vector<D3D12_SIGNATURE_PARAMETER_DESC>& input_paramters)
    {
        std::vector<D3D12_INPUT_ELEMENT_DESC> vec_input_elements;

        uint32_t offset{ 0 };
        for (auto& paramter : input_paramters)
        {
            D3D12_INPUT_ELEMENT_DESC input_elem_desc{};
            input_elem_desc.SemanticName = SwitchSemanticName(paramter.SemanticName);
            input_elem_desc.SemanticIndex = paramter.SemanticIndex;
            input_elem_desc.Format = GetDxgiFormatFromSemanticName(paramter.SemanticName);
            input_elem_desc.InputSlot = 0;
            input_elem_desc.AlignedByteOffset = offset;
            input_elem_desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            input_elem_desc.InstanceDataStepRate = 0;
            offset += GetDxgiFormatTypeLength(input_elem_desc.Format);
            vec_input_elements.push_back(input_elem_desc);
        }

        return vec_input_elements;
    }

    void D3D12BoundResourceManager::InitializeBoundResource(ID3DBlob *const shader_arr[5])
    {
        std::array<TmpBindPointData, 4> tmp_bind_data;
        for (uint32_t i = 0; i < 5; i++)
        {
            ID3DBlob* shader_blob = shader_arr[i];
            if (shader_blob == nullptr)
            {
                continue;
            }

            ComPtr<ID3D12ShaderReflection> vs_shader_reflect;
            ThrowIfFailed(D3DReflect(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), IID_PPV_ARGS(&vs_shader_reflect)));

            D3D12_SHADER_DESC shader_desc{};
            ThrowIfFailed(vs_shader_reflect->GetDesc(&shader_desc));

            if (i == VS)
            {
                input_paramters_.resize(shader_desc.InputParameters);
                for (uint32_t i = 0; i < shader_desc.InputParameters; i++)
                {
                    ThrowIfFailed(vs_shader_reflect->GetInputParameterDesc(i, &input_paramters_[i]));
                }

                input_elements_ = ParserVsInputParamters(input_paramters_);
            }

            for (uint32_t r = 0; r < shader_desc.BoundResources; r++)
            {
                D3D12_SHADER_INPUT_BIND_DESC bound_resource_desc{};
                ThrowIfFailed(vs_shader_reflect->GetResourceBindingDesc(r, &bound_resource_desc));

                auto res_name = GetShaderResourceIdentify(bound_resource_desc.Name);
                D3D12_DESCRIPTOR_RANGE_TYPE descriptor_range_type = GetDescriptorRangeType(bound_resource_desc.Type);

                auto& resource_bind_desc = bound_point_map_[descriptor_range_type];
                auto& tmp_data = tmp_bind_data[descriptor_range_type];

                resource_bind_desc.range_type = descriptor_range_type;

                auto& res_bind = resource_bind_map_[res_name];
                res_bind.bind_desc = bound_resource_desc;
                res_bind.descriptor_range_desc = &resource_bind_desc;

                UpdateResourceBoundPoint(tmp_data, resource_bind_desc, bound_resource_desc);
            }
        }
    }

    void D3D12BoundResourceManager::InitializeDescriptorHeap()
    {
        int srv_uav_cbv_count = bound_point_map_[D3D12_DESCRIPTOR_RANGE_TYPE_SRV].bind_count +
            bound_point_map_[D3D12_DESCRIPTOR_RANGE_TYPE_UAV].bind_count +
            bound_point_map_[D3D12_DESCRIPTOR_RANGE_TYPE_CBV].bind_count;

        srv_uav_cbv_heap_ = D3D12Manager::CreateDescriptorHeap(srv_uav_cbv_count, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

        sampler_heap_ = D3D12Manager::CreateDescriptorHeap(bound_point_map_[D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER].bind_count, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    }

    void D3D12BoundResourceManager::InitializeRootSignature()
    {
        uint32_t paramter_count{ 1 };
        auto descriptor_ranges = GenerateDescriptorRange(bound_point_map_);

        D3D12_ROOT_PARAMETER root_paramter[2];
        root_paramter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_paramter[0].DescriptorTable.pDescriptorRanges = descriptor_ranges.data();
        root_paramter[0].DescriptorTable.NumDescriptorRanges = descriptor_ranges.size();
        root_paramter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        auto& sampler_descriptor = bound_point_map_[D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER];
        if (sampler_descriptor.bind_count > 0)
        {
            D3D12_DESCRIPTOR_RANGE sampler_range{};
            sampler_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            sampler_range.NumDescriptors = sampler_descriptor.bind_count;
            sampler_range.BaseShaderRegister = sampler_descriptor.bind_point;
            sampler_range.RegisterSpace = sampler_descriptor.space;
            sampler_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            root_paramter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            root_paramter[1].DescriptorTable.pDescriptorRanges = &sampler_range;
            root_paramter[1].DescriptorTable.NumDescriptorRanges = 1;
            root_paramter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            paramter_count++;
        }

        root_signature_ = D3D12Manager::CreateRootSignature(root_paramter, paramter_count);
    }

    DXGI_FORMAT D3D12BoundResourceManager::GetDxgiFormatFromSemanticName(const std::string& semnatic_name)
    {
        if (semnatic_name.find("POSITION") != std::string::npos)
        {
            return DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (semnatic_name.find("NORMAL") != std::string::npos)
        {
            return DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (semnatic_name.find("TANGENT") != std::string::npos)
        {
            return DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (semnatic_name.find("TEXCOORD") != std::string::npos)
        {
            return DXGI_FORMAT_R32G32_FLOAT;
        }

        ThrowIfFalse(0);

        return DXGI_FORMAT_UNKNOWN;
    }

    std::string D3D12BoundResourceManager::GetShaderResourceIdentify(const std::string& raw_name)
    {
        static std::regex id_reg("(\\w+)");
        std::smatch m;
        if (std::regex_search(raw_name, m, id_reg))
        {
            return m[0].str();
        }
        else
        {
            return "";
        }
    }

    D3D12_DESCRIPTOR_RANGE_TYPE D3D12BoundResourceManager::GetDescriptorRangeType(D3D_SHADER_INPUT_TYPE shader_input_type)
    {
        switch (shader_input_type)
        {
        case D3D_SIT_CBUFFER:
            return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

        case D3D_SIT_TBUFFER:
        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        case D3D_SIT_SAMPLER:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;


        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_BYTEADDRESS:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        case D3D_SIT_RTACCELERATIONSTRUCTURE:
        case D3D_SIT_UAV_FEEDBACKTEXTURE:

        default:
            ThrowIfFalse(0);
        break;
        }

        ThrowIfFalse(0);
        return D3D12_DESCRIPTOR_RANGE_TYPE();
    }

    void D3D12BoundResourceManager::UpdateResourceBoundPoint(TmpBindPointData& tmp_bind_point_data, DescriptorRangeBindPointDesc& resource_bound_desc, const D3D12_SHADER_INPUT_BIND_DESC& shader_input_desc)
    {
       tmp_bind_point_data.min_bind_point = (std::min)(shader_input_desc.BindPoint, tmp_bind_point_data.min_bind_point);
       tmp_bind_point_data.max_bind_point = (std::max)(shader_input_desc.BindPoint, tmp_bind_point_data.max_bind_point);

       resource_bound_desc.bind_point = tmp_bind_point_data.min_bind_point;
       resource_bound_desc.bind_count = tmp_bind_point_data.max_bind_point - tmp_bind_point_data.min_bind_point + 1;
    }

    std::vector<D3D12_DESCRIPTOR_RANGE> D3D12BoundResourceManager::GenerateDescriptorRange(RangeBindPointDescArray& range_bind_array)
    {
        std::vector<D3D12_DESCRIPTOR_RANGE> ret;
        uint32_t root_signature_offset{ 0 };

        for (auto& range_bind_point_desc : range_bind_array)
        {
            if (range_bind_point_desc.bind_count > 0 &&
                range_bind_point_desc.range_type != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
            {
                D3D12_DESCRIPTOR_RANGE range{};
                range.RangeType = range_bind_point_desc.range_type;
                range.NumDescriptors = range_bind_point_desc.bind_count;
                range.BaseShaderRegister = range_bind_point_desc.bind_point;
                range.RegisterSpace = range_bind_point_desc.space;
                range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                ret.push_back(range);
                range_bind_point_desc.root_signature_offset = root_signature_offset;
                root_signature_offset += range_bind_point_desc.bind_count;
            }
        }

        return ret;
    }

    const char* D3D12BoundResourceManager::SwitchSemanticName(const std::string& semantic_name)
    {
        auto it = std::find(SEMANTIC_NAMES.begin(), SEMANTIC_NAMES.end(), semantic_name);
        ThrowIfFailed(it != SEMANTIC_NAMES.end());

        return it->c_str();
    }

    D3D12_SAMPLER_DESC D3D12BoundResourceManager::GetDefaultSamplerDesc(DefaultSamplerType default_sampler)
    {
        switch (default_sampler)
        {
            case D3D::D3D12BoundResourceManager::kPointWrap:
               return
               {
                   D3D12_FILTER_MIN_MAG_MIP_POINT,
                   D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                   D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                   D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                   0.0f,
                   16,
                   D3D12_COMPARISON_FUNC_LESS_EQUAL,
                   D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
                   0.0f,
                   D3D12_FLOAT32_MAX
               };

            case D3D::D3D12BoundResourceManager::kPointClamp:
                return
                {
                   D3D12_FILTER_MIN_MAG_MIP_POINT,
                   D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                   D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                   D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                   0.0f,
                   16,
                   D3D12_COMPARISON_FUNC_LESS_EQUAL,
                   D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
                   0.0f,
                   D3D12_FLOAT32_MAX,
                };

            case D3D::D3D12BoundResourceManager::kLinearWrap:
                return
                {
                    D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                    D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                    D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                    D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                    0.0f,
                    16,
                    D3D12_COMPARISON_FUNC_LESS_EQUAL,
                    D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
                    0.0f,
                    D3D12_FLOAT32_MAX,
                };

            case D3D::D3D12BoundResourceManager::kLinearClamp:
            return
            {
               D3D12_FILTER_MIN_MAG_MIP_LINEAR,
               D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
               D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
               D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
               0.0f,
               16,
               D3D12_COMPARISON_FUNC_LESS_EQUAL,
               D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
               0.0f,
               D3D12_FLOAT32_MAX,
            };

            case D3D::D3D12BoundResourceManager::kAnisotropicWrap:
            return
            {
                D3D12_FILTER_ANISOTROPIC,
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                0.0f,
                8,
                D3D12_COMPARISON_FUNC_LESS_EQUAL,
                D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
                0.0f,
                D3D12_FLOAT32_MAX,
            };

            case D3D::D3D12BoundResourceManager::kAnisotropicClamp:
            return
            {
                 D3D12_FILTER_ANISOTROPIC,
                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                 0.0f,
                 8,
                 D3D12_COMPARISON_FUNC_LESS_EQUAL,
                 D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
                 0.0f,
                 D3D12_FLOAT32_MAX,
            };
        }

        ThrowIfFalse(0);

        return {};
   }
}

