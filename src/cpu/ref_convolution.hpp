/*******************************************************************************
* Copyright 2016-2018 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef CPU_REF_CONVOLUTION_HPP
#define CPU_REF_CONVOLUTION_HPP

#include <assert.h>

#include "c_types_map.hpp"
#include "type_helpers.hpp"
#include "utils.hpp"

#include "cpu_convolution_pd.hpp"
#include "cpu_primitive.hpp"

namespace mkldnn {
namespace impl {
namespace cpu {

template <impl::data_type_t src_type,
         impl::data_type_t wei_type = src_type,
         impl::data_type_t dst_type = src_type,
         impl::data_type_t acc_type = dst_type>
struct ref_convolution_fwd_t: public cpu_primitive_t {
    struct pd_t: public cpu_convolution_fwd_pd_t {
        using cpu_convolution_fwd_pd_t::cpu_convolution_fwd_pd_t;

        DECLARE_COMMON_PD_T("ref:any", ref_convolution_fwd_t);

        status_t init() {
            using namespace data_type;

            bool ok = true
                && this->set_default_params() == status::success
                && is_fwd()
                && utils::one_of(this->desc()->alg_kind,
                           alg_kind::convolution_auto,
                           alg_kind::convolution_direct)
                && this->desc()->src_desc.data_type == src_type
                && this->desc()->weights_desc.data_type == wei_type
                && this->desc()->accum_data_type == acc_type
                && this->desc()->dst_desc.data_type == dst_type
                && IMPLICATION(this->with_bias(), true
                        && IMPLICATION(src_type == u8,
                            utils::one_of(this->desc()->bias_desc.data_type,
                                f32, s32, s8, u8))
                        && IMPLICATION(src_type == f32,
                            this->desc()->bias_desc.data_type == f32))
                && this->attr()->has_default_values();
            return ok ? status::success : status::unimplemented;
        }
    };

    ref_convolution_fwd_t(const pd_t *apd): cpu_primitive_t(apd) {}

    typedef typename prec_traits<src_type>::type src_data_t;
    typedef typename prec_traits<wei_type>::type wei_data_t;
    typedef typename prec_traits<dst_type>::type dst_data_t;
    typedef typename prec_traits<acc_type>::type acc_data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        execute_forward(ctx);
        return status::success;
    }

private:
    void execute_forward(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_t::pd(); }
};

template <impl::data_type_t diff_src_type, impl::data_type_t wei_type,
         impl::data_type_t diff_dst_type,
         impl::data_type_t acc_type = diff_src_type>
struct ref_convolution_bwd_data_t: public cpu_primitive_t {
    struct pd_t: public cpu_convolution_bwd_data_pd_t {
        using cpu_convolution_bwd_data_pd_t::cpu_convolution_bwd_data_pd_t;

        DECLARE_COMMON_PD_T("ref:any", ref_convolution_bwd_data_t);

        status_t init() {
            bool ok = true
                && this->set_default_params() == status::success
                && this->desc()->prop_kind == prop_kind::backward_data
                && utils::one_of(this->desc()->alg_kind,
                           alg_kind::convolution_auto,
                           alg_kind::convolution_direct)
                && this->desc()->diff_dst_desc.data_type == diff_dst_type
                && this->desc()->weights_desc.data_type == wei_type
                && this->desc()->accum_data_type == acc_type
                && this->desc()->diff_src_desc.data_type == diff_src_type
                && this->attr()->has_default_values();
            return ok ? status::success : status::unimplemented;
        }

        virtual bool support_bias() const override { return true; }
    };

    ref_convolution_bwd_data_t(const pd_t *apd): cpu_primitive_t(apd) {}

    typedef typename prec_traits<diff_src_type>::type diff_src_data_t;
    typedef typename prec_traits<wei_type>::type wei_data_t;
    typedef typename prec_traits<diff_dst_type>::type diff_dst_data_t;
    typedef typename prec_traits<acc_type>::type acc_data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        execute_backward_data(ctx);
        return status::success;
    }

private:
    void execute_backward_data(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_t::pd(); }
};

template <impl::data_type_t src_type, impl::data_type_t diff_wei_type,
         impl::data_type_t diff_dst_type,
         impl::data_type_t acc_type = diff_wei_type>
struct ref_convolution_bwd_weights_t: public cpu_primitive_t {
    struct pd_t: public cpu_convolution_bwd_weights_pd_t {
        using cpu_convolution_bwd_weights_pd_t::cpu_convolution_bwd_weights_pd_t;

        DECLARE_COMMON_PD_T("ref:any", ref_convolution_bwd_weights_t);

        status_t init() {
            bool ok = true
                && this->set_default_params() == status::success
                && this->desc()->prop_kind == prop_kind::backward_weights
                && utils::one_of(this->desc()->alg_kind,
                           alg_kind::convolution_auto,
                           alg_kind::convolution_direct)
                && this->desc()->src_desc.data_type == src_type
                && this->desc()->diff_weights_desc.data_type == diff_wei_type
                && this->desc()->diff_dst_desc.data_type == diff_dst_type
                && this->desc()->accum_data_type == acc_type
                && IMPLICATION(this->with_bias(),
                        this->desc()->diff_bias_desc.data_type
                        == diff_wei_type)
                && this->attr()->has_default_values();
            return ok ? status::success : status::unimplemented;
        }
    };

    ref_convolution_bwd_weights_t(const pd_t *apd): cpu_primitive_t(apd) {}

    typedef typename prec_traits<src_type>::type src_data_t;
    typedef typename prec_traits<diff_wei_type>::type diff_wei_data_t;
    typedef typename prec_traits<diff_dst_type>::type diff_dst_data_t;
    typedef typename prec_traits<acc_type>::type acc_data_t;

    virtual status_t execute(const exec_ctx_t &ctx) const override {
        execute_backward_weights(ctx);
        return status::success;
    }

private:
    void execute_backward_weights(const exec_ctx_t &ctx) const;
    const pd_t *pd() const { return (const pd_t *)primitive_t::pd(); }
};

}
}
}

#endif

// vim: et ts=4 sw=4 cindent cino^=l0,\:0,N-s
