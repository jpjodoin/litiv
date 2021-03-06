
// This file is part of the LITIV framework; visit the original repository at
// https://github.com/plstcharles/litiv for more information.
//
// Copyright 2016 Pierre-Luc St-Charles; pierre-luc.st-charles<at>polymtl.ca
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "litiv/utils/platform.hpp"
#include "litiv/utils/console.hpp"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-local-typedefs"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wpedantic"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wextra"
#elif (defined(__GNUC__) || defined(__GNUG__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wextra"
#elif defined(_MSC_VER)
#pragma warning(push,0)
#endif //defined(_MSC_VER)
#ifndef SYS_MEMORYINFO_ON
#define SYS_MEMORYINFO_ON
#endif //ndef(SYS_MEMORYINFO_ON)
#if OPENGM_ENABLE_FAST_DEBUG_MAT_OPS
#ifndef NDEBUG
#define ADDED_NDEBUG
#define NDEBUG
#endif //ndef(NDEBUG)
#endif //OPENGM_ENABLE_FAST_DEBUG_MAT_OPS
#ifndef HAVE_CPP0X_VARIADIC_TEMPLATES
#define HAVE_CPP0X_VARIADIC_TEMPLATES
#endif //ndef(HAVE_CPP0X_VARIADIC_TEMPLATES)
#include <opengm/graphicalmodel/graphicalmodel.hxx>
#include <opengm/functions/potts.hxx>
#include <opengm/functions/explicit_function.hxx>
#include <opengm/graphicalmodel/space/simplediscretespace.hxx>
#include <opengm/graphicalmodel/space/static_simplediscretespace.hxx>
#include <opengm/inference/inference.hxx>
#include <opengm/inference/visitors/visitors.hxx>
#if HAVE_OPENGM_EXTLIB
#if HAVE_OPENGM_EXTLIB_QPBO
#include <opengm/inference/fix-fusion/higher-order-energy.hpp>
#include <opengm/inference/external/qpbo/QPBO.h>
#endif //HAVE_OPENGM_EXTLIB_QPBO
#if HAVE_OPENGM_EXTLIB_FASTPD
#include <opengm/inference/external/fastPD.hxx>
#endif //HAVE_OPENGM_EXTLIB_FASTPD
#endif //HAVE_OPENGM_EXTLIB
#ifdef ADDED_NDEBUG
#undef ADDED_NDEBUG
#undef NDEBUG
#endif //def(ADDED_NDEBUG)
#if defined(_MSC_VER)
#pragma warning(pop)
#elif (defined(__GNUC__) || defined(__GNUG__))
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif //defined(__clang__)
#if HAVE_BOOST
#include "litiv/3rdparty/sospd/sospd.hpp"
#endif //HAVE_BOOST

namespace lv {

    namespace gm {

        /// prints general information about a graphical model
        template<typename GraphModelType>
        inline void printModelInfo(const GraphModelType& oGM) {
            size_t nMinVarLabelCount = SIZE_MAX, nMaxVarLabelCount = 0;
            for(size_t v=0; v<oGM.numberOfVariables(); ++v) {
                nMinVarLabelCount = std::min(oGM.numberOfLabels(v),nMinVarLabelCount);
                nMaxVarLabelCount = std::max(oGM.numberOfLabels(v),nMaxVarLabelCount);
            }
            std::map<size_t,size_t> mFactOrderHist;
            for(size_t f=0; f<oGM.numberOfFactors(); ++f)
                mFactOrderHist[oGM.operator[](f).numberOfVariables()] += 1;
            lvAssert_(std::accumulate(mFactOrderHist.begin(),mFactOrderHist.end(),size_t(0),[](const size_t n, const auto& p){return n+p.second;})==oGM.numberOfFactors(),"factor count mismatch");
            lvCout << "\tmodel has " << oGM.numberOfVariables() << " variables (" << ((nMinVarLabelCount==nMaxVarLabelCount)?std::to_string(nMinVarLabelCount)+" labels each)":std::to_string(nMinVarLabelCount)+" labels min, "+std::to_string(nMaxVarLabelCount)+" labels max)") << '\n';
            lvCout << "\tmodel has " << oGM.numberOfFactors() << " factors;\n";
            for(const auto& oOrderBin : mFactOrderHist) {
                lvCout << "\t\t" << oOrderBin.second << " factors w/ order=" << oOrderBin.first << '\n';
            }
        }

        /// explicit view function wrapper to bypass marray allocations and use views instead (interface similar to opengm::ExplicitFunction's)
        template<typename TValue, typename TIndex=size_t, typename TLabel=size_t>
        struct ExplicitViewFunction :
                public marray::View<TValue>,
                public opengm::FunctionBase<ExplicitViewFunction<TValue,TIndex,TLabel>,TValue,TIndex,TLabel> {
            /// default constructor (null view data)
            ExplicitViewFunction() : marray::View<TValue>() {}
            /// copy constructor (this will point to other's view data)
            ExplicitViewFunction(const ExplicitViewFunction& other) : marray::View<TValue>(other) {}
            /// assignment operation (this will point to other's view data)
            ExplicitViewFunction& operator=(const ExplicitViewFunction& other) {
                marray::View<TValue>::operator=(other);
                return *this;
            }
            /// empty data assignment (resets internal view struct)
            void assign() {
                this->marray::View<TValue>::assign();
            }
            /// view data assignment (note: data will be accessed in last-idx-major format)
            template<class TShapeIterator>
            void assign(TShapeIterator begin, TShapeIterator end, TValue* data) {
                this->marray::View<TValue>::assign(begin,end,data);
            }
        };

        /// explicit scaled view function wrapper (same as above, but scales all costs by user-defined factor)
        template<typename TValue, typename TIndex=size_t, typename TLabel=size_t, typename TScale=float>
        struct ExplicitScaledViewFunction : ExplicitViewFunction<TValue,TIndex,TLabel> {
            /// default constructor (null view data)
            explicit ExplicitScaledViewFunction(TScale fScale=TScale(1)) : ExplicitViewFunction<TValue,TIndex,TLabel>(),m_fScale(fScale) {}
            /// copy constructor (this will point to other's view data)
            ExplicitScaledViewFunction(const ExplicitScaledViewFunction& other, TScale fScale=TScale(1)) : ExplicitViewFunction<TValue,TIndex,TLabel>(other),m_fScale(fScale) {}
            /// assignment operation (this will point to other's view data)
            ExplicitScaledViewFunction& operator=(const ExplicitScaledViewFunction& other) {
                marray::View<TValue>::operator=(other);
                this->m_fScale = other.m_fScale;
                return *this;
            }
            /// internal scale factor getter
            TScale getScale() const {return m_fScale;}
            /// internal scale factor setter
            void setScale(TScale fScale) {m_fScale = fScale;}
            /// element access function (applies scale)
            template<class U>
            TValue operator()(U u) const {
                return TValue(m_fScale*this->ExplicitViewFunction<TValue,TIndex,TLabel>::operator()(u));
            }
            /// element access function (applies scale)
            TValue operator()(const size_t value) const {
                return TValue(m_fScale*this->ExplicitViewFunction<TValue,TIndex,TLabel>::operator()(value));
            }
            /// element access function (applies scale)
            template<typename... TArgs>
            TValue operator()(const size_t value, const TArgs... args) const {
                return TValue(m_fScale*this->ExplicitViewFunction<TValue,TIndex,TLabel>::operator()(value,args...));
            }
        private:
            TScale m_fScale;
        };

        /// clique interface used used for faster factor/energy/nodes access through graph model
        /// note: for optimal performance, users that know the clique's size should cast, and use derived public members instead of virtual functions
        template<typename TValue, typename TIndex=size_t, typename TLabel=size_t>
        struct IClique {
            /// returns whether the clique is initialized/in use or not
            virtual bool isValid() const = 0;
            /// returns the clique size for the derived object
            virtual TIndex getSize() const = 0;
            /// returns the graph factor index for this clique
            virtual TIndex getFactorId() const = 0;
            /// returns this clique's function tied to its graph factor
            virtual void* getFunctionPtr() const = 0;
            /// returns a LUT node index for a member of this clique
            virtual TIndex getLUTNodeIdx(TIndex nInternalIdx) const = 0;
            /// returns the beginning LUT node iterator for this clique
            virtual const TIndex* getLUTNodeIter() const = 0;
            /// returns a graph node index for a member of this clique
            virtual TIndex getGraphNodeIdx(TIndex nInternalIdx) const = 0;
            /// returns the beginning graph node iterator for this clique
            virtual const TIndex* getGraphNodeIter() const = 0;
            /// implicit bool conversion operator returns whether this clique is valid or not
            operator bool() const {return isValid();}
        };

        /// clique implementation used for faster factor/energy/nodes access through graph model
        /// note: for optimal performance, users that know the clique's size should use public members instead of virtual functions
        template<size_t nSize, typename TValue, typename TIndex=size_t, typename TLabel=size_t, typename ENABLE=void>
        struct Clique;

        /// clique implementation used for faster factor/energy/nodes access through graph model (with non-null size specialization)
        /// note: for optimal performance, users that know the clique's size should use public members instead of virtual functions
        template<size_t nSize, typename TValue, typename TIndex, typename TLabel>
        struct Clique<nSize,TValue,TIndex,TLabel,std::enable_if_t<(nSize>size_t(0))>> :
                IClique<TValue,TIndex,TLabel> {
            /// default constructor; fills all members with invalid values that must be specified later
            Clique() {
                m_bValid = false;
                m_nGraphFactorId = std::numeric_limits<TIndex>::max();
                m_pGraphFunctionPtr = nullptr;
                std::fill_n(m_anLUTNodeIdxs.begin(),s_nCliqueSize,std::numeric_limits<TIndex>::max());
                std::fill_n(m_anGraphNodeIdxs.begin(),s_nCliqueSize,std::numeric_limits<TIndex>::max());
            }
            /// returns whether the clique is initialized/in use or not through the virtual interface
            virtual bool isValid() const override final {
                return m_bValid;
            }
            /// returns the clique size of this object through the virtual interface (for casting)
            virtual TIndex getSize() const override final {
                return s_nCliqueSize;
            }
            /// returns the graph factor index for this clique through the virtual interface
            virtual TIndex getFactorId() const override final {
                return m_nGraphFactorId;
            }
            /// returns this clique's function tied to its graph factor through the virtual interface
            virtual void* getFunctionPtr() const override final {
                return m_pGraphFunctionPtr;
            }
            /// returns a LUT node index for a member of this clique through the virtual interface
            virtual TIndex getLUTNodeIdx(TIndex nInternalIdx) const override final {
                lvDbgAssert(nInternalIdx<s_nCliqueSize);
                return m_anLUTNodeIdxs[nInternalIdx];
            }
            /// returns the beginning LUT node iterator for this clique
            virtual const TIndex* getLUTNodeIter() const override final {
                return m_anLUTNodeIdxs.begin();
            }
            /// returns the in-graph index of a node in this clique through the virtual interface
            virtual TIndex getGraphNodeIdx(TIndex nInternalIdx) const override final {
                lvDbgAssert(nInternalIdx<s_nCliqueSize);
                return m_anGraphNodeIdxs[nInternalIdx];
            }
            /// returns the beginning graph node iterator for this clique
            virtual const TIndex* getGraphNodeIter() const override final {
                return m_anGraphNodeIdxs.begin();
            }
            /// implicit bool conversion operator returns whether this clique is valid or not
            operator bool() const {
                return m_bValid;
            }
            /// specifies whether this clique is valid or not (i.e. initialized and/or used)
            bool m_bValid;
            /// graph factor id for this clique
            TIndex m_nGraphFactorId;
            /// graph factor function for this clique (user must cast it to correct function type)
            void* m_pGraphFunctionPtr;
            /// LUT node indices for members of this clique (useful when mapping ROIs to a graph)
            std::array<TIndex,nSize> m_anLUTNodeIdxs;
            /// graph node indices for members of this clique (should always be a valid index in gmodel)
            std::array<TIndex,nSize> m_anGraphNodeIdxs;
            /// static size member of this clique (for type traits ops)
            static constexpr TIndex s_nCliqueSize = TIndex(nSize);
        };

        /// clique implementation used for faster factor/energy/nodes access through graph model (with null size specialization)
        /// note: for optimal performance, users that know the clique's size should use public members instead of virtual functions
        template<size_t nSize, typename TValue, typename TIndex, typename TLabel>
        struct Clique<nSize,TValue,TIndex,TLabel,std::enable_if_t<(nSize==size_t(0))>> :
                IClique<TValue,TIndex,TLabel> {
            /// default constructor (empty for null-sized clique)
            Clique() = default;
            /// returns whether the clique is initialized/in use or not through the virtual interface
            virtual bool isValid() const override final {
                return false;
            }
            /// returns the clique size of this object through the virtual interface (for casting)
            virtual TIndex getSize() const override final {
                return TIndex(0);
            }
            /// returns the graph factor index for this clique through the virtual interface
            virtual TIndex getFactorId() const override final {
                return std::numeric_limits<TIndex>::max();
            }
            /// returns this clique's function tied to its graph factor through the virtual interface
            virtual void* getFunctionPtr() const override final {
                return nullptr;
            }
            /// returns a LUT node index for a member of this clique through the virtual interface
            virtual TIndex getLUTNodeIdx(TIndex) const override final {
                return std::numeric_limits<TIndex>::max();
            }
            /// returns the beginning LUT node iterator for this clique
            virtual const TIndex* getLUTNodeIter() const override final {
                return nullptr;
            }
            /// returns the in-graph index of a node in this clique through the virtual interface
            virtual TIndex getGraphNodeIdx(TIndex) const override final {
                return std::numeric_limits<TIndex>::max();
            }
            /// returns the beginning graph node iterator for this clique
            virtual const TIndex* getGraphNodeIter() const override final {
                return nullptr;
            }
            /// implicit bool conversion operator returns whether this clique is valid or not
            operator bool() const {
                return false;
            }
            /// static size member of this clique (for type traits ops)
            static constexpr TIndex s_nCliqueSize = TIndex(0);
        };

        /// higher-order term reducer (used by FGBZ solver w/ QPBO-compatible interface)
        template<typename TFunc, size_t nOrder, typename TValue, typename TIndex, typename TLabel, typename ReducerType>
        inline void factorReducer(const Clique<nOrder,TValue,TIndex,TLabel>& oClique, ReducerType& oReducer, TLabel nAlphaLabel, const TLabel* aLabeling) {
            if(oClique) {
                std::array<typename ReducerType::VarId,nOrder> aTermEnergyLUT;
                std::array<TLabel,nOrder> aCliqueLabels;
                std::array<TValue,(1u<<nOrder)> aCliqueCoeffs{};
                constexpr size_t nAssignCount = 1UL<<nOrder;
                for(size_t nAssignIdx=0; nAssignIdx<nAssignCount; ++nAssignIdx) {
                    for(size_t nVarIdx=0; nVarIdx<nOrder; ++nVarIdx)
                        aCliqueLabels[nVarIdx] = (nAssignIdx&(1<<nVarIdx))?nAlphaLabel:aLabeling[oClique.m_anLUTNodeIdxs[nVarIdx]];
                    for(size_t nAssignSubsetIdx=1; nAssignSubsetIdx<nAssignCount; ++nAssignSubsetIdx) {
                        if(!(nAssignIdx&~nAssignSubsetIdx)) {
                            int nParityBit = 0;
                            for(size_t nVarIdx=0; nVarIdx<nOrder; ++nVarIdx)
                                nParityBit ^= (((nAssignIdx^nAssignSubsetIdx)&(1<<nVarIdx))!=0);
                            lvDbgAssert(oClique.m_pGraphFunctionPtr);
                            const TValue fCurrAssignEnergy = (*(TFunc*)oClique.m_pGraphFunctionPtr)(aCliqueLabels.begin());
                            aCliqueCoeffs[nAssignSubsetIdx] += nParityBit?-fCurrAssignEnergy:fCurrAssignEnergy;
                        }
                    }
                }
                for(size_t nAssignSubsetIdx=1; nAssignSubsetIdx<nAssignCount; ++nAssignSubsetIdx) {
                    int nCurrTermDegree = 0;
                    for(size_t nVarIdx=0; nVarIdx<nOrder; ++nVarIdx)
                        if(nAssignSubsetIdx&(1<<nVarIdx))
                            aTermEnergyLUT[nCurrTermDegree++] = (typename ReducerType::VarId)oClique.m_anGraphNodeIdxs[nVarIdx];
                    std::sort(aTermEnergyLUT.begin(),aTermEnergyLUT.begin()+nCurrTermDegree);
                    oReducer.AddTerm(aCliqueCoeffs[nAssignSubsetIdx],nCurrTermDegree,aTermEnergyLUT.data());
                }
            }
        }

        /// higher-order term reducer (opengm variation; used by FGBZ solver w/ QPBO-compatible interface)
        template<size_t nMaxOrder, typename ValueType, typename LabelType, typename FactorType, typename ReducerType>
        inline void factorReducer(FactorType& oGraphFactor, size_t nFactOrder, ReducerType& oReducer, LabelType nAlphaLabel, const size_t* pValidLUTNodeIdxs, const LabelType* aLabeling) {
            lvDbgAssert(oGraphFactor.numberOfVariables()==nFactOrder);
            std::array<typename ReducerType::VarId,nMaxOrder> aTermEnergyLUT;
            std::array<LabelType,nMaxOrder> aCliqueLabels;
            std::array<ValueType,(1u<<nMaxOrder)> aCliqueCoeffs;
            const size_t nAssignCount = 1UL<<nFactOrder;
            std::fill_n(aCliqueCoeffs.begin(),nAssignCount,ValueType(0));
            for(size_t nAssignIdx=0; nAssignIdx<nAssignCount; ++nAssignIdx) {
                for(size_t nVarIdx=0; nVarIdx<nFactOrder; ++nVarIdx)
                    aCliqueLabels[nVarIdx] = (nAssignIdx&(1<<nVarIdx))?nAlphaLabel:aLabeling[pValidLUTNodeIdxs[oGraphFactor.variableIndex(nVarIdx)]];
                for(size_t nAssignSubsetIdx=1; nAssignSubsetIdx<nAssignCount; ++nAssignSubsetIdx) {
                    if(!(nAssignIdx&~nAssignSubsetIdx)) {
                        int nParityBit = 0;
                        for(size_t nVarIdx=0; nVarIdx<nFactOrder; ++nVarIdx)
                            nParityBit ^= (((nAssignIdx^nAssignSubsetIdx)&(1<<nVarIdx))!=0);
                        const ValueType fCurrAssignEnergy = oGraphFactor(aCliqueLabels.begin());
                        aCliqueCoeffs[nAssignSubsetIdx] += nParityBit?-fCurrAssignEnergy:fCurrAssignEnergy;
                    }
                }
            }
            for(size_t nAssignSubsetIdx=1; nAssignSubsetIdx<nAssignCount; ++nAssignSubsetIdx) {
                int nCurrTermDegree = 0;
                for(size_t nVarIdx=0; nVarIdx<nFactOrder; ++nVarIdx)
                    if(nAssignSubsetIdx&(1<<nVarIdx))
                        aTermEnergyLUT[nCurrTermDegree++] = (typename ReducerType::VarId)oGraphFactor.variableIndex(nVarIdx);
                std::sort(aTermEnergyLUT.begin(),aTermEnergyLUT.begin()+nCurrTermDegree);
                oReducer.AddTerm(aCliqueCoeffs[nAssignSubsetIdx],nCurrTermDegree,aTermEnergyLUT.data());
            }
        }

    } // namespace gm

} // namespace lv

template<size_t nSize, typename TValue, typename TIndex, typename TLabel>
constexpr TIndex lv::gm::Clique<nSize,TValue,TIndex,TLabel,std::enable_if_t<(nSize>size_t(0))>>::s_nCliqueSize;

template<size_t nSize, typename TValue, typename TIndex, typename TLabel>
constexpr TIndex lv::gm::Clique<nSize,TValue,TIndex,TLabel,std::enable_if_t<(nSize==size_t(0))>>::s_nCliqueSize;