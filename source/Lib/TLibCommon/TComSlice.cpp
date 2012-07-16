/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TComSlice.cpp
    \brief    slice header and SPS class
*/

#include "CommonDef.h"
#include "TComSlice.h"
#include "TComPic.h"
#include "TLibEncoder/TEncSbac.h"
#include "TLibDecoder/TDecSbac.h"


//! \ingroup TLibCommon
//! \{

TComSlice::TComSlice()
: m_iPPSId                        ( -1 )
, m_iPOC                          ( 0 )
, m_iLastIDR                      ( 0 )
, m_eNalUnitType                  ( NAL_UNIT_CODED_SLICE_IDR )
, m_eSliceType                    ( I_SLICE )
, m_iSliceQp                      ( 0 )
#if ADAPTIVE_QP_SELECTION
, m_iSliceQpBase                  ( 0 )
#endif
, m_bLoopFilterDisable            ( false )
#if DBL_HL_SYNTAX
, m_inheritDblParamFromPPS       ( true )
#else
, m_inheritDblParamFromAPS       ( true )
#endif
, m_loopFilterBetaOffsetDiv2    ( 0 )
, m_loopFilterTcOffsetDiv2      ( 0 )
, m_bRefPicListModificationFlagLC ( false )
, m_bRefPicListCombinationFlag    ( false )
, m_bCheckLDC                     ( false )
, m_iSliceQpDelta                 ( 0 )
, m_iDepth                        ( 0 )
, m_bRefenced                     ( false )
, m_pcSPS                         ( NULL )
, m_pcPPS                         ( NULL )
, m_pcPic                         ( NULL )
, m_uiColDir                      ( 0 )
, m_colRefIdx                     ( 0 )
#if ALF_CHROMA_LAMBDA || SAO_CHROMA_LAMBDA
, m_dLambdaLuma( 0.0 )
, m_dLambdaChroma( 0.0 )
#else
, m_dLambda                       ( 0.0 )
#endif
, m_bNoBackPredFlag               ( false )
#if !REMOVE_LC
, m_bRefIdxCombineCoding          ( false )
#endif
, m_uiTLayer                      ( 0 )
, m_bTLayerSwitchingFlag          ( false )
, m_uiSliceMode                   ( 0 )
, m_uiSliceArgument               ( 0 )
, m_uiSliceCurStartCUAddr         ( 0 )
, m_uiSliceCurEndCUAddr           ( 0 )
, m_uiSliceIdx                    ( 0 )
, m_uiDependentSliceMode            ( 0 )
, m_uiDependentSliceArgument        ( 0 )
, m_uiDependentSliceCurStartCUAddr  ( 0 )
, m_uiDependentSliceCurEndCUAddr    ( 0 )
, m_bNextSlice                    ( false )
, m_bNextDependentSlice             ( false )
, m_uiSliceBits                   ( 0 )
, m_uiDependentSliceCounter         ( 0 )
, m_bFinalized                    ( false )
, m_uiTileByteLocation            ( NULL )
, m_uiTileCount                   ( 0 )
#if !REMOVE_TILE_MARKERS
, m_iTileMarkerFlag               ( 0 )
#endif
, m_uiTileOffstForMultES          ( 0 )
, m_puiSubstreamSizes             ( NULL )
, m_cabacInitFlag                 ( false )
, m_numEntryPointOffsets          ( 0 )
#if CODE_POCLSBLT_FIXEDLEN
, m_nalRefFlag                    ( 0 )
#endif
#if SLICE_TMVP_ENABLE
, m_enableTMVPFlag                ( true )
#endif
{
  m_aiNumRefIdx[0] = m_aiNumRefIdx[1] = m_aiNumRefIdx[2] = 0;
  
  initEqualRef();
  
  for(Int iNumCount = 0; iNumCount < MAX_NUM_REF_LC; iNumCount++)
  {
    m_iRefIdxOfLC[REF_PIC_LIST_0][iNumCount]=-1;
    m_iRefIdxOfLC[REF_PIC_LIST_1][iNumCount]=-1;
    m_eListIdFromIdxOfLC[iNumCount]=0;
    m_iRefIdxFromIdxOfLC[iNumCount]=0;
    m_iRefIdxOfL0FromRefIdxOfL1[iNumCount] = -1;
    m_iRefIdxOfL1FromRefIdxOfL0[iNumCount] = -1;
  }    
  for(Int iNumCount = 0; iNumCount < MAX_NUM_REF; iNumCount++)
  {
    m_apcRefPicList [0][iNumCount] = NULL;
    m_apcRefPicList [1][iNumCount] = NULL;
    m_aiRefPOCList  [0][iNumCount] = 0;
    m_aiRefPOCList  [1][iNumCount] = 0;
  }
  m_bCombineWithReferenceFlag = 0;
  resetWpScaling(m_weightPredTable);
#if !REMOVE_LC
  resetWpScalingLC(m_weightPredTableLC);
#endif
  initWpAcDcParam();
#if H0391_LF_ACROSS_SLICE_BOUNDARY_CONTROL
  m_saoEnabledFlag = false;
  m_alfEnabledFlag[0] = m_alfEnabledFlag[1] = m_alfEnabledFlag[2] = false;
#endif
}

TComSlice::~TComSlice()
{
  if (m_uiTileByteLocation) 
  {
    delete [] m_uiTileByteLocation;
    m_uiTileByteLocation = NULL;
  }
  delete[] m_puiSubstreamSizes;
  m_puiSubstreamSizes = NULL;
#if DEPENDENT_SLICES
  for (std::vector<TEncSbac*>::iterator i = CTXMem_enc.begin(); i != CTXMem_enc.end(); i++)
  {
    delete (*i);
  }
  CTXMem_enc.clear();
  for (std::vector<TDecSbac*>::iterator i = CTXMem_dec.begin(); i != CTXMem_dec.end(); i++)
  {
    delete (*i);
  }
  CTXMem_dec.clear();
#endif
}

Void TComSlice::initCTXMem_enc(  UInt i )                
{   
  for (std::vector<TEncSbac*>::iterator j = CTXMem_enc.begin(); j != CTXMem_enc.end(); j++)
  {
    delete (*j);
  }
  CTXMem_enc.clear(); 
  CTXMem_enc.resize(i); 
}

Void TComSlice::initCTXMem_dec(  UInt i )                
{   
  for (std::vector<TDecSbac*>::iterator j = CTXMem_dec.begin(); j != CTXMem_dec.end(); j++)
  {
    delete (*j);
  }
  CTXMem_dec.clear(); 
  CTXMem_dec.resize(i); 
}


Void TComSlice::initSlice()
{
  m_aiNumRefIdx[0]      = 0;
  m_aiNumRefIdx[1]      = 0;
  
  m_uiColDir = 0;
  
  m_colRefIdx = 0;
  initEqualRef();
  m_bNoBackPredFlag = false;
#if !REMOVE_LC
  m_bRefIdxCombineCoding = false;
#endif
  m_bRefPicListCombinationFlag = false;
  m_bRefPicListModificationFlagLC = false;
  m_bCheckLDC = false;

  m_aiNumRefIdx[REF_PIC_LIST_C]      = 0;

  m_uiMaxNumMergeCand = MRG_MAX_NUM_CANDS_SIGNALED;

  m_bFinalized=false;

  m_uiTileCount          = 0;
  m_cabacInitFlag        = false;
  m_numEntryPointOffsets = 0;
#if SLICE_TMVP_ENABLE
  m_enableTMVPFlag = true;
#endif
#if DEPENDENT_SLICES
  CTXMem_enc.clear();
  CTXMem_dec.clear();
#endif
}

Void TComSlice::initTiles()
{
  Int iWidth             = m_pcSPS->getPicWidthInLumaSamples();
  Int iHeight            = m_pcSPS->getPicHeightInLumaSamples();
  UInt uiWidthInCU       = ( iWidth %g_uiMaxCUWidth  ) ? iWidth /g_uiMaxCUWidth  + 1 : iWidth /g_uiMaxCUWidth;
  UInt uiHeightInCU      = ( iHeight%g_uiMaxCUHeight ) ? iHeight/g_uiMaxCUHeight + 1 : iHeight/g_uiMaxCUHeight;
  UInt uiNumCUsInFrame   = uiWidthInCU * uiHeightInCU;

  if (m_uiTileByteLocation==NULL) m_uiTileByteLocation   = new UInt[uiNumCUsInFrame];
}


/**
 - allocate table to contain substream sizes to be written to the slice header.
 .
 \param uiNumSubstreams Number of substreams -- the allocation will be this value - 1.
 */
Void  TComSlice::allocSubstreamSizes(UInt uiNumSubstreams)
{
  delete[] m_puiSubstreamSizes;
  m_puiSubstreamSizes = new UInt[uiNumSubstreams > 0 ? uiNumSubstreams-1 : 0];
}

Void  TComSlice::sortPicList        (TComList<TComPic*>& rcListPic)
{
  TComPic*    pcPicExtract;
  TComPic*    pcPicInsert;
  
  TComList<TComPic*>::iterator    iterPicExtract;
  TComList<TComPic*>::iterator    iterPicExtract_1;
  TComList<TComPic*>::iterator    iterPicInsert;
  
  for (Int i = 1; i < (Int)(rcListPic.size()); i++)
  {
    iterPicExtract = rcListPic.begin();
    for (Int j = 0; j < i; j++) iterPicExtract++;
    pcPicExtract = *(iterPicExtract);
    pcPicExtract->setCurrSliceIdx(0);
    
    iterPicInsert = rcListPic.begin();
    while (iterPicInsert != iterPicExtract)
    {
      pcPicInsert = *(iterPicInsert);
      pcPicInsert->setCurrSliceIdx(0);
      if (pcPicInsert->getPOC() >= pcPicExtract->getPOC())
      {
        break;
      }
      
      iterPicInsert++;
    }
    
    iterPicExtract_1 = iterPicExtract;    iterPicExtract_1++;
    
    //  swap iterPicExtract and iterPicInsert, iterPicExtract = curr. / iterPicInsert = insertion position
    rcListPic.insert (iterPicInsert, iterPicExtract, iterPicExtract_1);
    rcListPic.erase  (iterPicExtract);
  }
}

TComPic* TComSlice::xGetRefPic (TComList<TComPic*>& rcListPic,
                                UInt                uiPOC)
{
  TComList<TComPic*>::iterator  iterPic = rcListPic.begin();  
  TComPic*                      pcPic = *(iterPic);
  while ( iterPic != rcListPic.end() )
  {
    if(pcPic->getPOC() == uiPOC)
      break;
    iterPic++;
    pcPic = *(iterPic);
  }
  return  pcPic;
}


TComPic* TComSlice::xGetLongTermRefPic (TComList<TComPic*>& rcListPic,
                                UInt                uiPOC)
{
  TComList<TComPic*>::iterator  iterPic = rcListPic.begin();  
  TComPic*                      pcPic = *(iterPic);
  TComPic*                      pcStPic = pcPic;
  while ( iterPic != rcListPic.end() )
  {
    pcPic = *(iterPic);
    if(pcPic && (pcPic->getPOC()%(1<<getSPS()->getBitsForPOC())) == (uiPOC%(1<<getSPS()->getBitsForPOC())))
    {
      if(pcPic->getIsLongTerm())
        return pcPic;
      else
        pcStPic = pcPic;
      break;
    }

    iterPic++;
  }
  return  pcStPic;
}

Void TComSlice::setRefPOCList       ()
{
  for (Int iDir = 0; iDir < 2; iDir++)
  {
    for (Int iNumRefIdx = 0; iNumRefIdx < m_aiNumRefIdx[iDir]; iNumRefIdx++)
    {
      m_aiRefPOCList[iDir][iNumRefIdx] = m_apcRefPicList[iDir][iNumRefIdx]->getPOC();
    }
  }

}

Void TComSlice::generateCombinedList()
{
  if(m_aiNumRefIdx[REF_PIC_LIST_C] > 0)
  {
    m_aiNumRefIdx[REF_PIC_LIST_C]=0;
    for(Int iNumCount = 0; iNumCount < MAX_NUM_REF_LC; iNumCount++)
    {
      m_iRefIdxOfLC[REF_PIC_LIST_0][iNumCount]=-1;
      m_iRefIdxOfLC[REF_PIC_LIST_1][iNumCount]=-1;
      m_eListIdFromIdxOfLC[iNumCount]=0;
      m_iRefIdxFromIdxOfLC[iNumCount]=0;
      m_iRefIdxOfL0FromRefIdxOfL1[iNumCount] = -1;
      m_iRefIdxOfL1FromRefIdxOfL0[iNumCount] = -1;
    }

    for (Int iNumRefIdx = 0; iNumRefIdx < MAX_NUM_REF; iNumRefIdx++)
    {
      if(iNumRefIdx < m_aiNumRefIdx[REF_PIC_LIST_0])
      {
        Bool bTempRefIdxInL2 = true;
        for ( Int iRefIdxLC = 0; iRefIdxLC < m_aiNumRefIdx[REF_PIC_LIST_C]; iRefIdxLC++ )
        {
          if ( m_apcRefPicList[REF_PIC_LIST_0][iNumRefIdx]->getPOC() == m_apcRefPicList[m_eListIdFromIdxOfLC[iRefIdxLC]][m_iRefIdxFromIdxOfLC[iRefIdxLC]]->getPOC() )
          {
            m_iRefIdxOfL1FromRefIdxOfL0[iNumRefIdx] = m_iRefIdxFromIdxOfLC[iRefIdxLC];
            m_iRefIdxOfL0FromRefIdxOfL1[m_iRefIdxFromIdxOfLC[iRefIdxLC]] = iNumRefIdx;
            bTempRefIdxInL2 = false;
            break;
          }
        }

        if(bTempRefIdxInL2 == true)
        { 
          m_eListIdFromIdxOfLC[m_aiNumRefIdx[REF_PIC_LIST_C]] = REF_PIC_LIST_0;
          m_iRefIdxFromIdxOfLC[m_aiNumRefIdx[REF_PIC_LIST_C]] = iNumRefIdx;
          m_iRefIdxOfLC[REF_PIC_LIST_0][iNumRefIdx] = m_aiNumRefIdx[REF_PIC_LIST_C]++;
        }
      }

      if(iNumRefIdx < m_aiNumRefIdx[REF_PIC_LIST_1])
      {
        Bool bTempRefIdxInL2 = true;
        for ( Int iRefIdxLC = 0; iRefIdxLC < m_aiNumRefIdx[REF_PIC_LIST_C]; iRefIdxLC++ )
        {
          if ( m_apcRefPicList[REF_PIC_LIST_1][iNumRefIdx]->getPOC() == m_apcRefPicList[m_eListIdFromIdxOfLC[iRefIdxLC]][m_iRefIdxFromIdxOfLC[iRefIdxLC]]->getPOC() )
          {
            m_iRefIdxOfL0FromRefIdxOfL1[iNumRefIdx] = m_iRefIdxFromIdxOfLC[iRefIdxLC];
            m_iRefIdxOfL1FromRefIdxOfL0[m_iRefIdxFromIdxOfLC[iRefIdxLC]] = iNumRefIdx;
            bTempRefIdxInL2 = false;
            break;
          }
        }
        if(bTempRefIdxInL2 == true)
        {
          m_eListIdFromIdxOfLC[m_aiNumRefIdx[REF_PIC_LIST_C]] = REF_PIC_LIST_1;
          m_iRefIdxFromIdxOfLC[m_aiNumRefIdx[REF_PIC_LIST_C]] = iNumRefIdx;
          m_iRefIdxOfLC[REF_PIC_LIST_1][iNumRefIdx] = m_aiNumRefIdx[REF_PIC_LIST_C]++;
        }
      }
    }
  }
}

Void TComSlice::setRefPicList( TComList<TComPic*>& rcListPic )
{
  if (m_eSliceType == I_SLICE)
  {
    ::memset( m_apcRefPicList, 0, sizeof (m_apcRefPicList));
    ::memset( m_aiNumRefIdx,   0, sizeof ( m_aiNumRefIdx ));
    
    return;
  }
  
  m_aiNumRefIdx[0] = getNumRefIdx(REF_PIC_LIST_0);
  m_aiNumRefIdx[1] = getNumRefIdx(REF_PIC_LIST_1);

  TComPic*  pcRefPic= NULL;
  TComPic*  RefPicSetStCurr0[16];
  TComPic*  RefPicSetStCurr1[16];
  TComPic*  RefPicSetLtCurr[16];
  UInt NumPocStCurr0 = 0;
  UInt NumPocStCurr1 = 0;
  UInt NumPocLtCurr = 0;
  Int i;

  for(i=0; i < m_pcRPS->getNumberOfNegativePictures(); i++)
  {
    if(m_pcRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pcRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
#if NO_MV_SCALING_IF_LONG_TERM_REF
      pcRefPic->setIsUsedAsLongTerm(0);
#endif
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr0[NumPocStCurr0] = pcRefPic;
      NumPocStCurr0++;
      pcRefPic->setCheckLTMSBPresent(false);  
    }
  }
  for(; i < m_pcRPS->getNumberOfNegativePictures()+m_pcRPS->getNumberOfPositivePictures(); i++)
  {
    if(m_pcRPS->getUsed(i))
    {
      pcRefPic = xGetRefPic(rcListPic, getPOC()+m_pcRPS->getDeltaPOC(i));
      pcRefPic->setIsLongTerm(0);
#if NO_MV_SCALING_IF_LONG_TERM_REF
      pcRefPic->setIsUsedAsLongTerm(0);
#endif
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetStCurr1[NumPocStCurr1] = pcRefPic;
      NumPocStCurr1++;
      pcRefPic->setCheckLTMSBPresent(false);  
    }
  }
  for(i = m_pcRPS->getNumberOfNegativePictures()+m_pcRPS->getNumberOfPositivePictures()+m_pcRPS->getNumberOfLongtermPictures()-1; i > m_pcRPS->getNumberOfNegativePictures()+m_pcRPS->getNumberOfPositivePictures()-1 ; i--)
  {
    if(m_pcRPS->getUsed(i))
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pcRPS->getPOC(i));
      pcRefPic->setIsLongTerm(1);
#if NO_MV_SCALING_IF_LONG_TERM_REF
      pcRefPic->setIsUsedAsLongTerm(1);
#endif
      pcRefPic->getPicYuvRec()->extendPicBorder();
      RefPicSetLtCurr[NumPocLtCurr] = pcRefPic;
      NumPocLtCurr++;
    }
    if(pcRefPic==NULL) 
    {
      pcRefPic = xGetLongTermRefPic(rcListPic, m_pcRPS->getPOC(i));
    }
    pcRefPic->setCheckLTMSBPresent(m_pcRPS->getCheckLTMSBPresent(i));  
  }

  // ref_pic_list_init
  UInt cIdx = 0;
  UInt num_ref_idx_l0_active_minus1 = m_aiNumRefIdx[0] - 1;
  UInt num_ref_idx_l1_active_minus1 = m_aiNumRefIdx[1] - 1;
  TComPic*  refPicListTemp0[MAX_NUM_REF+1];
  TComPic*  refPicListTemp1[MAX_NUM_REF+1];
  Int  numRpsCurrTempList0, numRpsCurrTempList1;
  
  numRpsCurrTempList0 = numRpsCurrTempList1 = NumPocStCurr0 + NumPocStCurr1 + NumPocLtCurr;
  if (numRpsCurrTempList0 <= num_ref_idx_l0_active_minus1)
  {
    numRpsCurrTempList0 = num_ref_idx_l0_active_minus1 + 1;
  }
  if (numRpsCurrTempList1 <= num_ref_idx_l1_active_minus1)
  {
    numRpsCurrTempList1 = num_ref_idx_l1_active_minus1 + 1;
  }

  cIdx = 0;
  while (cIdx < numRpsCurrTempList0)
  {
    for ( i=0; i<NumPocStCurr0 && cIdx<numRpsCurrTempList0; cIdx++,i++)
    {
      refPicListTemp0[cIdx] = RefPicSetStCurr0[ i ];
    }
    for ( i=0; i<NumPocStCurr1 && cIdx<numRpsCurrTempList0; cIdx++,i++)
    {
      refPicListTemp0[cIdx] = RefPicSetStCurr1[ i ];
    }
    for ( i=0; i<NumPocLtCurr && cIdx<numRpsCurrTempList0; cIdx++,i++)
    {
      refPicListTemp0[cIdx] = RefPicSetLtCurr[ i ];
    }
  }
  cIdx = 0;
  while (cIdx<numRpsCurrTempList1 && m_eSliceType==B_SLICE)
  {
    for ( i=0; i<NumPocStCurr1 && cIdx<numRpsCurrTempList1; cIdx++,i++)
    {
      refPicListTemp1[cIdx] = RefPicSetStCurr1[ i ];
    }
    for ( i=0; i<NumPocStCurr0 && cIdx<numRpsCurrTempList1; cIdx++,i++)
    {
      refPicListTemp1[cIdx] = RefPicSetStCurr0[ i ];
    }
    for ( i=0; i<NumPocLtCurr && cIdx<numRpsCurrTempList1; cIdx++,i++)
    {
      refPicListTemp1[cIdx] = RefPicSetLtCurr[ i ];
    }
  }

  for (cIdx = 0; cIdx <= num_ref_idx_l0_active_minus1; cIdx ++)
  {
    m_apcRefPicList[0][cIdx] = m_RefPicListModification.getRefPicListModificationFlagL0() ? refPicListTemp0[ m_RefPicListModification.getRefPicSetIdxL0(cIdx) ] : refPicListTemp0[cIdx];
  }
  if ( m_eSliceType == P_SLICE )
  {
    m_aiNumRefIdx[1] = 0;
    ::memset( m_apcRefPicList[1], 0, sizeof(m_apcRefPicList[1]));
  }
  else
  {
    for (cIdx = 0; cIdx <= num_ref_idx_l1_active_minus1; cIdx ++)
    {
      m_apcRefPicList[1][cIdx] = m_RefPicListModification.getRefPicListModificationFlagL1() ? refPicListTemp1[ m_RefPicListModification.getRefPicSetIdxL1(cIdx) ] : refPicListTemp1[cIdx];
    }
  }
}

Int TComSlice::getNumRpsCurrTempList()
{
  Int numRpsCurrTempList = 0;

  if (m_eSliceType == I_SLICE) 
  {
    return 0;
  }
  for(UInt i=0; i < m_pcRPS->getNumberOfNegativePictures()+ m_pcRPS->getNumberOfPositivePictures() + m_pcRPS->getNumberOfLongtermPictures(); i++)
  {
    if(m_pcRPS->getUsed(i))
    {
      numRpsCurrTempList++;
    }
  }
  return numRpsCurrTempList;
}

Void TComSlice::initEqualRef()
{
  for (Int iDir = 0; iDir < 2; iDir++)
  {
    for (Int iRefIdx1 = 0; iRefIdx1 < MAX_NUM_REF; iRefIdx1++)
    {
      for (Int iRefIdx2 = iRefIdx1; iRefIdx2 < MAX_NUM_REF; iRefIdx2++)
      {
        m_abEqualRef[iDir][iRefIdx1][iRefIdx2] = m_abEqualRef[iDir][iRefIdx2][iRefIdx1] = (iRefIdx1 == iRefIdx2? true : false);
      }
    }
  }
}

Void TComSlice::checkColRefIdx(UInt curSliceIdx, TComPic* pic)
{
  Int i;
  TComSlice* curSlice = pic->getSlice(curSliceIdx);
  Int currColRefPOC =  curSlice->getRefPOC( RefPicList(curSlice->getColDir()), curSlice->getColRefIdx());
  TComSlice* preSlice;
  Int preColRefPOC;
  for(i=curSliceIdx-1; i>=0; i--)
  {
    preSlice = pic->getSlice(i);
    if(preSlice->getSliceType() != I_SLICE)
    {
      preColRefPOC  = preSlice->getRefPOC( RefPicList(preSlice->getColDir()), preSlice->getColRefIdx());
      if(currColRefPOC != preColRefPOC)
      {
        printf("Collocated_ref_idx shall always be the same for all slices of a coded picture!\n");
        exit(EXIT_FAILURE);
      }
      else
      {
        break;
      }
    }
  }
}

#if CRA_BLA_TFD_MODIFICATIONS
Void TComSlice::checkCRA(TComReferencePictureSet *pReferencePictureSet, Int& pocCRA, Bool& prevRAPisBLA, TComList<TComPic*>& rcListPic)
#else
Void TComSlice::checkCRA(TComReferencePictureSet *pReferencePictureSet, Int& pocCRA, TComList<TComPic*>& rcListPic)
#endif
{
  for(Int i = 0; i < pReferencePictureSet->getNumberOfNegativePictures()+pReferencePictureSet->getNumberOfPositivePictures(); i++)
  {
    if(pocCRA < MAX_UINT && getPOC() > pocCRA)
    {
      assert(getPOC()+pReferencePictureSet->getDeltaPOC(i) >= pocCRA);
    }
  }
  for(Int i = pReferencePictureSet->getNumberOfNegativePictures()+pReferencePictureSet->getNumberOfPositivePictures(); i < pReferencePictureSet->getNumberOfPictures(); i++)
  {
    if(pocCRA < MAX_UINT && getPOC() > pocCRA)
    {
      assert(pReferencePictureSet->getPOC(i) >= pocCRA);
    }
  }
#if CRA_BLA_TFD_MODIFICATIONS
  if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR ) // IDR picture found
  {
    prevRAPisBLA = false;
  }
  else if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA || getNalUnitType() == NAL_UNIT_CODED_SLICE_CRANT ) // CRA/CRANT picture found
  {
    pocCRA = getPOC();
    prevRAPisBLA = false;
  }
  else if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLANT ) // BLA/BLANT picture found
  {
    pocCRA = getPOC();
    prevRAPisBLA = true;
  }
#else
  if (getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA) // CRA picture found
  {
    pocCRA = getPOC();
  }
#endif
}

#if CRA_BLA_TFD_MODIFICATIONS
/** Function for marking the reference pictures when an IDR/CRA/CRANT/BLA/BLANT is encountered.
 * \param pocCRA POC of the CRA/CRANT/BLA/BLANT picture
 * \param bRefreshPending flag indicating if a deferred decoding refresh is pending
 * \param rcListPic reference to the reference picture list
 * This function marks the reference pictures as "unused for reference" in the following conditions.
 * If the nal_unit_type is IDR/BLA/BLANT, all pictures in the reference picture list  
 * are marked as "unused for reference"
 *    If the nal_unit_type is BLA/BLANT, set the pocCRA to the temporal reference of the current picture.
 * Otherwise
 *    If the bRefreshPending flag is true (a deferred decoding refresh is pending) and the current 
 *    temporal reference is greater than the temporal reference of the latest CRA/CRANT/BLA/BLANT picture (pocCRA), 
 *    mark all reference pictures except the latest CRA/CRANT/BLA/BLANT picture as "unused for reference" and set 
 *    the bRefreshPending flag to false.
 *    If the nal_unit_type is CRA/CRANT, set the bRefreshPending flag to true and pocCRA to the temporal 
 *    reference of the current picture.
 * Note that the current picture is already placed in the reference list and its marking is not changed.
 * If the current picture has a nal_ref_idc that is not 0, it will remain marked as "used for reference".
 */
#else
/** Function for marking the reference pictures when an IDR and CRA is encountered.
 * \param pocCRA POC of the CRA picture
 * \param bRefreshPending flag indicating if a deferred decoding refresh is pending
 * \param rcListPic reference to the reference picture list
 * This function marks the reference pictures as "unused for reference" in the following conditions.
 * If the nal_unit_type is IDR all pictures in the reference picture list  
 * is marked as "unused for reference" 
 * Otherwise do for the CRA case (non CRA case has no effect since both if conditions below will not be true)
 *    If the bRefreshPending flag is true (a deferred decoding refresh is pending) and the current 
 *    temporal reference is greater than the temporal reference of the latest CRA picture (pocCRA), 
 *    mark all reference pictures except the latest CRA picture as "unused for reference" and set 
 *    the bRefreshPending flag to false.
 *    If the nal_unit_type is CRA, set the bRefreshPending flag to true and pocCRA to the temporal 
 *    reference of the current picture.
 * Note that the current picture is already placed in the reference list and its marking is not changed.
 * If the current picture has a nal_ref_idc that is not 0, it will remain marked as "used for reference".
 */
#endif
Void TComSlice::decodingRefreshMarking(Int& pocCRA, Bool& bRefreshPending, TComList<TComPic*>& rcListPic)
{
  TComPic*                 rpcPic;
  UInt uiPOCCurr = getPOC(); 

#if CRA_BLA_TFD_MODIFICATIONS
  if (getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLANT)  // IDR/BLA/BLANT
#else
  if (getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR)  // IDR
#endif
  {
    // mark all pictures as not used for reference
    TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic);
      rpcPic->setCurrSliceIdx(0);
      if (rpcPic->getPOC() != uiPOCCurr) rpcPic->getSlice(0)->setReferenced(false);
      iterPic++;
    }
#if CRA_BLA_TFD_MODIFICATIONS
    if (getNalUnitType() == NAL_UNIT_CODED_SLICE_BLA || getNalUnitType() == NAL_UNIT_CODED_SLICE_BLANT)
    {
      pocCRA = uiPOCCurr;
    }
#endif
  }
  else // CRA or No DR
  {
    if (bRefreshPending==true && uiPOCCurr > pocCRA) // CRA reference marking pending 
    {
      TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
      while (iterPic != rcListPic.end())
      {
        rpcPic = *(iterPic);
        if (rpcPic->getPOC() != uiPOCCurr && rpcPic->getPOC() != pocCRA) rpcPic->getSlice(0)->setReferenced(false);
        iterPic++;
      }
      bRefreshPending = false; 
    }
#if CRA_BLA_TFD_MODIFICATIONS
    if (getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA || getNalUnitType() == NAL_UNIT_CODED_SLICE_CRANT) // CRA/CRANT picture found
#else
    if (getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA) // CRA picture found
#endif
    {
      bRefreshPending = true; 
      pocCRA = uiPOCCurr;
    }
  }
}

Void TComSlice::copySliceInfo(TComSlice *pSrc)
{
  assert( pSrc != NULL );

  Int i, j, k;

  m_iPOC                 = pSrc->m_iPOC;
  m_eNalUnitType         = pSrc->m_eNalUnitType;
  m_eSliceType           = pSrc->m_eSliceType;
  m_iSliceQp             = pSrc->m_iSliceQp;
#if ADAPTIVE_QP_SELECTION
  m_iSliceQpBase         = pSrc->m_iSliceQpBase;
#endif
  m_bLoopFilterDisable   = pSrc->m_bLoopFilterDisable;
#if DBL_HL_SYNTAX
  m_inheritDblParamFromPPS = pSrc->m_inheritDblParamFromPPS;
#else
  m_inheritDblParamFromAPS = pSrc->m_inheritDblParamFromAPS;
#endif
  m_loopFilterBetaOffsetDiv2 = pSrc->m_loopFilterBetaOffsetDiv2;
  m_loopFilterTcOffsetDiv2 = pSrc->m_loopFilterTcOffsetDiv2;
  
  for (i = 0; i < 3; i++)
  {
    m_aiNumRefIdx[i]     = pSrc->m_aiNumRefIdx[i];
  }

  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < MAX_NUM_REF_LC; j++)
    {
       m_iRefIdxOfLC[i][j]  = pSrc->m_iRefIdxOfLC[i][j];
    }
  }
  for (i = 0; i < MAX_NUM_REF_LC; i++)
  {
    m_eListIdFromIdxOfLC[i] = pSrc->m_eListIdFromIdxOfLC[i];
    m_iRefIdxFromIdxOfLC[i] = pSrc->m_iRefIdxFromIdxOfLC[i];
    m_iRefIdxOfL1FromRefIdxOfL0[i] = pSrc->m_iRefIdxOfL1FromRefIdxOfL0[i];
    m_iRefIdxOfL0FromRefIdxOfL1[i] = pSrc->m_iRefIdxOfL0FromRefIdxOfL1[i];
  }
  m_bRefPicListModificationFlagLC = pSrc->m_bRefPicListModificationFlagLC;
  m_bRefPicListCombinationFlag    = pSrc->m_bRefPicListCombinationFlag;
  m_bCheckLDC             = pSrc->m_bCheckLDC;
  m_iSliceQpDelta        = pSrc->m_iSliceQpDelta;
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < MAX_NUM_REF; j++)
    {
      m_apcRefPicList[i][j]  = pSrc->m_apcRefPicList[i][j];
      m_aiRefPOCList[i][j]   = pSrc->m_aiRefPOCList[i][j];
    }
  }  
  m_iDepth               = pSrc->m_iDepth;

  // referenced slice
  m_bRefenced            = pSrc->m_bRefenced;

  // access channel
  m_pcSPS                = pSrc->m_pcSPS;
  m_pcPPS                = pSrc->m_pcPPS;
  m_pcRPS                = pSrc->m_pcRPS;
  m_iLastIDR             = pSrc->m_iLastIDR;

  m_pcPic                = pSrc->m_pcPic;
  m_pcAPS                = pSrc->m_pcAPS;
  m_iAPSId               = pSrc->m_iAPSId;

  m_uiColDir             = pSrc->m_uiColDir;
  m_colRefIdx            = pSrc->m_colRefIdx;
#if ALF_CHROMA_LAMBDA || SAO_CHROMA_LAMBDA 
  m_dLambdaLuma          = pSrc->m_dLambdaLuma;
  m_dLambdaChroma        = pSrc->m_dLambdaChroma;
#else
  m_dLambda              = pSrc->m_dLambda;
#endif
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < MAX_NUM_REF; j++)
    {
      for (k =0; k < MAX_NUM_REF; k++)
      {
        m_abEqualRef[i][j][k] = pSrc->m_abEqualRef[i][j][k];
      }
    }
  }

  m_bNoBackPredFlag      = pSrc->m_bNoBackPredFlag;
#if !REMOVE_LC
  m_bRefIdxCombineCoding = pSrc->m_bRefIdxCombineCoding;
#endif
  m_uiTLayer                      = pSrc->m_uiTLayer;
  m_bTLayerSwitchingFlag          = pSrc->m_bTLayerSwitchingFlag;

  m_uiSliceMode                   = pSrc->m_uiSliceMode;
  m_uiSliceArgument               = pSrc->m_uiSliceArgument;
  m_uiSliceCurStartCUAddr         = pSrc->m_uiSliceCurStartCUAddr;
  m_uiSliceCurEndCUAddr           = pSrc->m_uiSliceCurEndCUAddr;
  m_uiSliceIdx                    = pSrc->m_uiSliceIdx;
  m_uiDependentSliceMode            = pSrc->m_uiDependentSliceMode;
  m_uiDependentSliceArgument        = pSrc->m_uiDependentSliceArgument; 
  m_uiDependentSliceCurStartCUAddr  = pSrc->m_uiDependentSliceCurStartCUAddr;
  m_uiDependentSliceCurEndCUAddr    = pSrc->m_uiDependentSliceCurEndCUAddr;
  m_bNextSlice                    = pSrc->m_bNextSlice;
  m_bNextDependentSlice             = pSrc->m_bNextDependentSlice;
#if !REMOVE_TILE_MARKERS
  m_iTileMarkerFlag             = pSrc->m_iTileMarkerFlag;
#endif
  for ( int e=0 ; e<2 ; e++ )
    for ( int n=0 ; n<MAX_NUM_REF ; n++ )
      memcpy(m_weightPredTable[e][n], pSrc->m_weightPredTable[e][n], sizeof(wpScalingParam)*3 );

  m_saoEnabledFlag = pSrc->m_saoEnabledFlag; 
#if !SAO_REMOVE_APS // APS syntax
  m_saoInterleavingFlag = pSrc->m_saoInterleavingFlag;
#endif
  m_saoEnabledFlagCb = pSrc->m_saoEnabledFlagCb;
  m_saoEnabledFlagCr = pSrc->m_saoEnabledFlagCr; 
  m_cabacInitFlag                = pSrc->m_cabacInitFlag;
  m_numEntryPointOffsets  = pSrc->m_numEntryPointOffsets;

  m_bLMvdL1Zero = pSrc->m_bLMvdL1Zero;
#if AHG6_ALF_OPTION2
  for(Int compIdx=0; compIdx < 3; compIdx++)
  {
    m_alfEnabledFlag[compIdx] = pSrc->m_alfEnabledFlag[compIdx];
  }
#endif
#if H0391_LF_ACROSS_SLICE_BOUNDARY_CONTROL
  m_LFCrossSliceBoundaryFlag = pSrc->m_LFCrossSliceBoundaryFlag;
#endif
#if SLICE_TMVP_ENABLE
  m_enableTMVPFlag                = pSrc->m_enableTMVPFlag;
#endif
}

#if PREVREFPIC_DEFN
int TComSlice::m_prevPOC[MAX_TLAYER] = {0};
#else
int TComSlice::m_prevPOC = 0;
#endif
/** Function for setting the slice's temporal layer ID and corresponding temporal_layer_switching_point_flag.
 * \param uiTLayer Temporal layer ID of the current slice
 * The decoder calls this function to set temporal_layer_switching_point_flag for each temporal layer based on 
 * the SPS's temporal_id_nesting_flag and the parsed PPS.  Then, current slice's temporal layer ID and 
 * temporal_layer_switching_point_flag is set accordingly.
 */
Void TComSlice::setTLayerInfo( UInt uiTLayer )
{
  m_uiTLayer = uiTLayer;
}

/** Function for checking if this is a switching-point
*/
Bool TComSlice::isTemporalLayerSwitchingPoint( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet)
{
  TComPic* rpcPic;
  // loop through all pictures in the reference picture buffer
  TComList<TComPic*>::iterator iterPic = rcListPic.begin();
  while ( iterPic != rcListPic.end())
  {
    rpcPic = *(iterPic++);
    if(rpcPic->getSlice(0)->isReferenced() && rpcPic->getPOC() != getPOC())
    {
      if(rpcPic->getTLayer() >= getTLayer())
      {
        return false;
      }
    }
  }
  return true;
}

/** Function for applying picture marking based on the Reference Picture Set in pReferencePictureSet.
*/
Void TComSlice::applyReferencePictureSet( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet)
{
  TComPic* rpcPic;
  Int i, isReference;

  Int j = 0;
  // loop through all pictures in the reference picture buffer
  TComList<TComPic*>::iterator iterPic = rcListPic.begin();
  while ( iterPic != rcListPic.end())
  {
    j++;
    rpcPic = *(iterPic++);

    isReference = 0;
    // loop through all pictures in the Reference Picture Set
    // to see if the picture should be kept as reference picture
    for(i=0;i<pReferencePictureSet->getNumberOfPositivePictures()+pReferencePictureSet->getNumberOfNegativePictures();i++)
    {
      if(!rpcPic->getIsLongTerm() && rpcPic->getPicSym()->getSlice(0)->getPOC() == this->getPOC() + pReferencePictureSet->getDeltaPOC(i))
      {
        isReference = 1;
        rpcPic->setUsedByCurr(pReferencePictureSet->getUsed(i));
        rpcPic->setIsLongTerm(0);
#if NO_MV_SCALING_IF_LONG_TERM_REF
        rpcPic->setIsUsedAsLongTerm(0);
#endif
      }
    }
    for(;i<pReferencePictureSet->getNumberOfPictures();i++)
    {
      if(pReferencePictureSet->getCheckLTMSBPresent(i)==true)
      {
        if(rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice(0)->getPOC()) == pReferencePictureSet->getPOC(i))
        {
          isReference = 1;
          rpcPic->setUsedByCurr(pReferencePictureSet->getUsed(i));
        }
      }
      else 
      {
        if(rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice(0)->getPOC()%(1<<rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC())) == pReferencePictureSet->getPOC(i)%(1<<rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC()))
        {
          isReference = 1;
          rpcPic->setUsedByCurr(pReferencePictureSet->getUsed(i));
        }
      }

    }
    // mark the picture as "unused for reference" if it is not in
    // the Reference Picture Set
    if(rpcPic->getPicSym()->getSlice(0)->getPOC() != this->getPOC() && isReference == 0)    
    {            
      rpcPic->getSlice( 0 )->setReferenced( false );   
      rpcPic->setIsLongTerm(0);
    }
  }  
}

/** Function for applying picture marking based on the Reference Picture Set in pReferencePictureSet.
*/
Int TComSlice::checkThatAllRefPicsAreAvailable( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet, Bool printErrors, Int pocRandomAccess)
{
  TComPic* rpcPic;
  Int i, isAvailable, j;
  Int atLeastOneLost = 0;
  Int atLeastOneRemoved = 0;
  Int iPocLost = 0;

  // loop through all long-term pictures in the Reference Picture Set
  // to see if the picture should be kept as reference picture
  for(i=pReferencePictureSet->getNumberOfNegativePictures()+pReferencePictureSet->getNumberOfPositivePictures();i<pReferencePictureSet->getNumberOfPictures();i++)
  {
    j = 0;
    isAvailable = 0;
    // loop through all pictures in the reference picture buffer
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while ( iterPic != rcListPic.end())
    {
      j++;
      rpcPic = *(iterPic++);
      if(pReferencePictureSet->getCheckLTMSBPresent(i)==true)
      {
        if(rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice(0)->getPOC()) == pReferencePictureSet->getPOC(i) && rpcPic->getSlice(0)->isReferenced())
        {
          isAvailable = 1;
        }
      }
      else 
      {
        if(rpcPic->getIsLongTerm() && (rpcPic->getPicSym()->getSlice(0)->getPOC()%(1<<rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC())) == pReferencePictureSet->getPOC(i)%(1<<rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC()) && rpcPic->getSlice(0)->isReferenced())
        {
          isAvailable = 1;
        }
      }
    }
    // if there was no such long-term check the short terms
    if(!isAvailable)
    {
      iterPic = rcListPic.begin();
      while ( iterPic != rcListPic.end())
      {
        j++;
        rpcPic = *(iterPic++);

        if((rpcPic->getPicSym()->getSlice(0)->getPOC()%(1<<rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC())) == (this->getPOC() + pReferencePictureSet->getDeltaPOC(i))%(1<<rpcPic->getPicSym()->getSlice(0)->getSPS()->getBitsForPOC()) && rpcPic->getSlice(0)->isReferenced())
        {
          isAvailable = 1;
          rpcPic->setIsLongTerm(1);
#if NO_MV_SCALING_IF_LONG_TERM_REF
          rpcPic->setIsUsedAsLongTerm(1);
#endif
          break;
        }
      }
    }
    // report that a picture is lost if it is in the Reference Picture Set
    // but not available as reference picture
    if(isAvailable == 0)    
    {            
      if (this->getPOC() + pReferencePictureSet->getDeltaPOC(i) >= pocRandomAccess)
      {
        if(!pReferencePictureSet->getUsed(i) )
        {
          if(printErrors)
            printf("\nLong-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
          atLeastOneRemoved = 1;
        }
        else
        {
          if(printErrors)
            printf("\nLong-term reference picture with POC = %3d is lost or not correctly decoded!", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
          atLeastOneLost = 1;
          iPocLost=this->getPOC() + pReferencePictureSet->getDeltaPOC(i);
        }
      }
    }
  }  
  // loop through all short-term pictures in the Reference Picture Set
  // to see if the picture should be kept as reference picture
  for(i=0;i<pReferencePictureSet->getNumberOfNegativePictures()+pReferencePictureSet->getNumberOfPositivePictures();i++)
  {
    j = 0;
    isAvailable = 0;
    // loop through all pictures in the reference picture buffer
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while ( iterPic != rcListPic.end())
    {
      j++;
      rpcPic = *(iterPic++);

      if(!rpcPic->getIsLongTerm() && rpcPic->getPicSym()->getSlice(0)->getPOC() == this->getPOC() + pReferencePictureSet->getDeltaPOC(i) && rpcPic->getSlice(0)->isReferenced())
      {
        isAvailable = 1;
      }
    }
    // report that a picture is lost if it is in the Reference Picture Set
    // but not available as reference picture
    if(isAvailable == 0)    
    {            
      if (this->getPOC() + pReferencePictureSet->getDeltaPOC(i) >= pocRandomAccess)
      {
        if(!pReferencePictureSet->getUsed(i) )
        {
          if(printErrors)
            printf("\nShort-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
          atLeastOneRemoved = 1;
        }
        else
        {
          if(printErrors)
            printf("\nShort-term reference picture with POC = %3d is lost or not correctly decoded!", this->getPOC() + pReferencePictureSet->getDeltaPOC(i));
          atLeastOneLost = 1;
          iPocLost=this->getPOC() + pReferencePictureSet->getDeltaPOC(i);
        }
      }
    }
  }    
  if(atLeastOneLost)
  {
    return iPocLost+1;
  }
  if(atLeastOneRemoved)
  {
    return -2;
  }
  else
    return 0;
}

/** Function for constructing an explicit Reference Picture Set out of the available pictures in a referenced Reference Picture Set
*/
Void TComSlice::createExplicitReferencePictureSetFromReference( TComList<TComPic*>& rcListPic, TComReferencePictureSet *pReferencePictureSet)
{
  TComPic* rpcPic;
  Int i, j;
  Int k = 0;
  Int nrOfNegativePictures = 0;
  Int nrOfPositivePictures = 0;
  TComReferencePictureSet* pcRPS = this->getLocalRPS();

  // loop through all pictures in the Reference Picture Set
  for(i=0;i<pReferencePictureSet->getNumberOfPictures();i++)
  {
    j = 0;
    // loop through all pictures in the reference picture buffer
    TComList<TComPic*>::iterator iterPic = rcListPic.begin();
    while ( iterPic != rcListPic.end())
    {
      j++;
      rpcPic = *(iterPic++);

      if(rpcPic->getPicSym()->getSlice(0)->getPOC() == this->getPOC() + pReferencePictureSet->getDeltaPOC(i) && rpcPic->getSlice(0)->isReferenced())
      {
        // This picture exists as a reference picture
        // and should be added to the explicit Reference Picture Set
        pcRPS->setDeltaPOC(k, pReferencePictureSet->getDeltaPOC(i));
        pcRPS->setUsed(k, pReferencePictureSet->getUsed(i));
        if(pcRPS->getDeltaPOC(k) < 0)
          nrOfNegativePictures++;
        else
          nrOfPositivePictures++;
        k++;
      }
    }
  }
  pcRPS->setNumberOfNegativePictures(nrOfNegativePictures);
  pcRPS->setNumberOfPositivePictures(nrOfPositivePictures);
  pcRPS->setNumberOfPictures(nrOfNegativePictures+nrOfPositivePictures);
  // This is a simplistic inter rps example. A smarter encoder will look for a better reference RPS to do the 
  // inter RPS prediction with.  Here we just use the reference used by pReferencePictureSet.
  // If pReferencePictureSet is not inter_RPS_predicted, then inter_RPS_prediction is for the current RPS also disabled.
  if (!pReferencePictureSet->getInterRPSPrediction())
  {
    pcRPS->setInterRPSPrediction(false);
    pcRPS->setNumRefIdc(0);
  }
  else
  {
    Int rIdx =  this->getRPSidx() - pReferencePictureSet->getDeltaRIdxMinus1() - 1;
    Int deltaRPS = pReferencePictureSet->getDeltaRPS();
    TComReferencePictureSet* pcRefRPS = this->getSPS()->getRPSList()->getReferencePictureSet(rIdx);
    Int iRefPics = pcRefRPS->getNumberOfPictures();
    Int iNewIdc=0;
    for(i=0; i<= iRefPics; i++) 
    {
      Int deltaPOC = ((i != iRefPics)? pcRefRPS->getDeltaPOC(i) : 0);  // check if the reference abs POC is >= 0
      Int iRefIdc = 0;
      for (j=0; j < pcRPS->getNumberOfPictures(); j++) // loop through the  pictures in the new RPS
      {
        if ( (deltaPOC + deltaRPS) == pcRPS->getDeltaPOC(j))
        {
          if (pcRPS->getUsed(j))
          {
            iRefIdc = 1;
          }
          else
          {
            iRefIdc = 2;
          }
        }
      }
      pcRPS->setRefIdc(i, iRefIdc);
      iNewIdc++;
    }
    pcRPS->setInterRPSPrediction(true);
    pcRPS->setNumRefIdc(iNewIdc);
    pcRPS->setDeltaRPS(deltaRPS); 
    pcRPS->setDeltaRIdxMinus1(pReferencePictureSet->getDeltaRIdxMinus1() + this->getSPS()->getRPSList()->getNumberOfReferencePictureSets() - this->getRPSidx());
  }

  this->setRPS(pcRPS);
  this->setRPSidx(-1);
}
#if !SLICE_TMVP_ENABLE
Void TComSlice::decodingMarkingForNoTMVP( TComList<TComPic*>& rcListPic, Int currentPOC )
{
  TComList<TComPic*>::iterator it;
  for ( it = rcListPic.begin(); it != rcListPic.end(); it++ )
  {
    if ( (*it)->getSlice(0)->getPOC() != currentPOC )
    {
      (*it)->setUsedForTMVP( false );
    }
  }
}
#endif
/** get AC and DC values for weighted pred
 * \param *wp
 * \returns Void
 */
Void  TComSlice::getWpAcDcParam(wpACDCParam *&wp)
{
  wp = m_weightACDCParam;
}

/** init AC and DC values for weighted pred
 * \returns Void
 */
Void  TComSlice::initWpAcDcParam()
{
  for(Int iComp = 0; iComp < 3; iComp++ )
  {
    m_weightACDCParam[iComp].iAC = 0;
    m_weightACDCParam[iComp].iDC = 0;
  }
}

/** get WP tables for weighted pred
 * \param RefPicList
 * \param iRefIdx
 * \param *&wpScalingParam
 * \returns Void
 */
Void  TComSlice::getWpScaling( RefPicList e, Int iRefIdx, wpScalingParam *&wp )
{
  wp = m_weightPredTable[e][iRefIdx];
}

/** reset Default WP tables settings : no weight. 
 * \param wpScalingParam
 * \returns Void
 */
Void  TComSlice::resetWpScaling(wpScalingParam  wp[2][MAX_NUM_REF][3])
{
  for ( int e=0 ; e<2 ; e++ ) 
  {
    for ( int i=0 ; i<MAX_NUM_REF ; i++ )
    {
      for ( int yuv=0 ; yuv<3 ; yuv++ ) 
      {
        wpScalingParam  *pwp = &(wp[e][i][yuv]);
        pwp->bPresentFlag      = false;
        pwp->uiLog2WeightDenom = 0;
        pwp->uiLog2WeightDenom = 0;
        pwp->iWeight           = 1;
        pwp->iOffset           = 0;
      }
    }
  }
}

/** init WP table
 * \returns Void
 */
Void  TComSlice::initWpScaling()
{
  initWpScaling(m_weightPredTable);
}

/** set WP tables 
 * \param wpScalingParam
 * \returns Void
 */
Void  TComSlice::initWpScaling(wpScalingParam  wp[2][MAX_NUM_REF][3])
{
  for ( int e=0 ; e<2 ; e++ ) 
  {
    for ( int i=0 ; i<MAX_NUM_REF ; i++ )
    {
      for ( int yuv=0 ; yuv<3 ; yuv++ ) 
      {
        wpScalingParam  *pwp = &(wp[e][i][yuv]);
        if ( !pwp->bPresentFlag ) {
          // Inferring values not present :
          pwp->iWeight = (1 << pwp->uiLog2WeightDenom);
          pwp->iOffset = 0;
        }

        pwp->w      = pwp->iWeight;
        pwp->o      = pwp->iOffset * (1 << (g_uiBitDepth-8));
        pwp->shift  = pwp->uiLog2WeightDenom;
        pwp->round  = (pwp->uiLog2WeightDenom>=1) ? (1 << (pwp->uiLog2WeightDenom-1)) : (0);
      }
    }
  }
}
#if !REMOVE_LC
/** get WP tables for weighted pred of LC
 * \param iRefIdxLC
 * \param *&wpScalingParam
 * \returns Void
 */
Void TComSlice::getWpScalingLC( Int iRefIdx, wpScalingParam *&wp )
{
  wp = m_weightPredTableLC[iRefIdx];
}
/** reset Default WP tables settings for LC : no weight. 
 * \param wpScalingParam
 * \returns Void
 */
Void TComSlice::resetWpScalingLC(wpScalingParam  wp[2*MAX_NUM_REF][3])
{
  for ( int i=0 ; i<2*MAX_NUM_REF ; i++ )
  {
    for ( int yuv=0 ; yuv<3 ; yuv++ ) 
    {
      wpScalingParam  *pwp = &(wp[i][yuv]);
      pwp->bPresentFlag      = false;
      pwp->uiLog2WeightDenom = 0;
      pwp->uiLog2WeightDenom = 0;
      pwp->iWeight           = 1;
      pwp->iOffset           = 0;
    }
  }
}
/** set current WP tables settings for LC
 * \returns Void
 */
Void TComSlice::setWpParamforLC()
{
  for ( Int iRefIdx=0 ; iRefIdx<getNumRefIdx(REF_PIC_LIST_C) ; iRefIdx++ )
  {
    RefPicList eRefPicList = (RefPicList)getListIdFromIdxOfLC(iRefIdx);
    Int iCombRefIdx = getRefIdxFromIdxOfLC(iRefIdx);

    wpScalingParam  *wp_src, *wp_dst;
    getWpScalingLC(iRefIdx, wp_src);
    getWpScaling(eRefPicList, iCombRefIdx, wp_dst);
    copyWPtable(wp_src, wp_dst);

    if(eRefPicList == REF_PIC_LIST_0)
    {
      Int iRefIdxL1 = getRefIdxOfL1FromRefIdxOfL0(iCombRefIdx);
      if(iRefIdxL1 >= 0)
      {
        getWpScaling(REF_PIC_LIST_1, iRefIdxL1, wp_dst);
        copyWPtable(wp_src, wp_dst);
      }
    }
    if(eRefPicList == REF_PIC_LIST_1)
    {
      Int iRefIdxL0 = getRefIdxOfL0FromRefIdxOfL1(iCombRefIdx);
      if(iRefIdxL0 >= 0)
      {
        getWpScaling(REF_PIC_LIST_0, iRefIdxL0, wp_dst);
        copyWPtable(wp_src, wp_dst);
      }
    }
  }
  initWpScaling();
}
/** copy source WP tables to destination table for LC
 * \param wpScalingParam *&wp_src : source
 * \param wpScalingParam *&wp_dst : destination
 * \returns Void
 */
Void TComSlice::copyWPtable(wpScalingParam *&wp_src, wpScalingParam *&wp_dst)
{
  for ( Int iComp = 0; iComp < 3; iComp++ )
  {
    wp_dst[iComp].uiLog2WeightDenom = (iComp==0) ? wp_src[0].uiLog2WeightDenom : wp_src[1].uiLog2WeightDenom;
    wp_dst[iComp].bPresentFlag = wp_src[iComp].bPresentFlag;
    wp_dst[iComp].iWeight = wp_src[iComp].iWeight;
    wp_dst[iComp].iOffset = wp_src[iComp].iOffset;
  }
}
#endif

// ------------------------------------------------------------------------------------------------
// Video parameter set (VPS)
// ------------------------------------------------------------------------------------------------
#if VPS_INTEGRATION
TComVPS::TComVPS()
: m_VPSId                     (  0)
, m_uiMaxTLayers              (  1)
, m_uiMaxLayers               (  1)
, m_bTemporalIdNestingFlag    (false)
{

  for( Int i = 0; i < MAX_TLAYER; i++)
  {
    m_numReorderPics[i] = 0;
    m_uiMaxDecPicBuffering[i] = 0; 
    m_uiMaxLatencyIncrease[i] = 0;
  }
}

TComVPS::~TComVPS()
{


}

#endif

// ------------------------------------------------------------------------------------------------
// Sequence parameter set (SPS)
// ------------------------------------------------------------------------------------------------

TComSPS::TComSPS()
: m_SPSId                     (  0)
, m_ProfileSpace              (  0)
, m_ProfileIdc                (  0)
, m_ReservedIndicatorFlags    (  0)
, m_LevelIdc                  (  0)
, m_ProfileCompatibility      (  0)
#if VPS_INTEGRATION
, m_VPSId                     (  0)
#endif
, m_chromaFormatIdc           (CHROMA_420)
, m_uiMaxTLayers              (  1)
// Structure
, m_picWidthInLumaSamples     (352)
, m_picHeightInLumaSamples    (288)
, m_picCroppingFlag           (false)
, m_picCropLeftOffset         (  0)
, m_picCropRightOffset        (  0)
, m_picCropTopOffset          (  0)
, m_picCropBottomOffset       (  0) 
, m_uiMaxCUWidth              ( 32)
, m_uiMaxCUHeight             ( 32)
, m_uiMaxCUDepth              (  3)
, m_uiMinTrDepth              (  0)
, m_uiMaxTrDepth              (  1)
, m_bLongTermRefsPresent      (false)
, m_uiQuadtreeTULog2MaxSize   (  0)
, m_uiQuadtreeTULog2MinSize   (  0)
, m_uiQuadtreeTUMaxDepthInter (  0)
, m_uiQuadtreeTUMaxDepthIntra (  0)
// Tool list
, m_usePCM                   (false)
, m_pcmLog2MaxSize            (  5)
, m_uiPCMLog2MinSize          (  7)
#if !REMOVE_INTER_4X4
, m_bDisInter4x4              (  1)
#endif
, m_bUseALF                   (false)
#if !AHG6_ALF_OPTION2
, m_bALFCoefInSlice           (false)
#endif
, m_bUseLMChroma              (false)
#if INTRA_TRANSFORMSKIP
, m_useTansformSkip           (false)
, m_useTansformSkipFast       (false)
#endif
, m_bUseLComb                 (false)
#if !REMOVE_LC
, m_bLCMod                    (false)
#endif
, m_restrictedRefPicListsFlag   (  1)
, m_listsModificationPresentFlag(  0)
, m_uiBitDepth                (  8)
, m_uiBitIncrement            (  0)
, m_qpBDOffsetY               (  0)
, m_qpBDOffsetC               (  0)
#if LOSSLESS_CODING
, m_useLossless               (false)
#endif
, m_uiPCMBitDepthLuma         (  8)
, m_uiPCMBitDepthChroma       (  8)
, m_bPCMFilterDisableFlag     (false)
, m_uiBitsForPOC              (  8)
, m_uiMaxTrSize               ( 32)
, m_bLFCrossSliceBoundaryFlag (false)
, m_bUseSAO                   (false) 
#if !TILES_OR_ENTROPY_FIX
, m_bLFCrossTileBoundaryFlag  (false) 
, m_iUniformSpacingIdr        (  0 )
, m_iTileBoundaryIndependenceIdr (  0 )
, m_iNumColumnsMinus1         (  0 )
, m_puiColumnWidth            ( NULL )
, m_iNumRowsMinus1            (  0 )
, m_puiRowHeight              ( NULL )
#endif
, m_bTemporalIdNestingFlag    (false)
, m_scalingListEnabledFlag    (false)
#if !TILES_OR_ENTROPY_FIX
,  m_tilesOrEntropyCodingSyncIdc( 0 )
,  m_numSubstreams              ( 0 )
#endif
{
  // AMVP parameter
  ::memset( m_aeAMVPMode, 0, sizeof( m_aeAMVPMode ) );
  for ( Int i = 0; i < MAX_TLAYER; i++ )
  {
    m_uiMaxLatencyIncrease[i] = 0;
    m_uiMaxDecPicBuffering[i] = 0;
    m_numReorderPics[i]       = 0;
  }
#if SCALING_LIST_HL_SYNTAX
  m_scalingList = new TComScalingList;
#endif
}

TComSPS::~TComSPS()
{
#if !TILES_OR_ENTROPY_FIX
  if( m_iNumColumnsMinus1 > 0 && m_iUniformSpacingIdr == 0 )
  {
    delete [] m_puiColumnWidth; 
    m_puiColumnWidth = NULL;
  }
  if( m_iNumRowsMinus1 > 0 && m_iUniformSpacingIdr == 0 )
  {
    delete [] m_puiRowHeight;
    m_puiRowHeight = NULL;
  }
#endif
#if SCALING_LIST_HL_SYNTAX
  delete m_scalingList;
#endif
  m_RPSList.destroy();
}

Void  TComSPS::createRPSList( Int numRPS )
{ 
  m_RPSList.destroy();
  m_RPSList.create(numRPS);
}

const Int TComSPS::m_cropUnitX[]={1,2,2,1};
const Int TComSPS::m_cropUnitY[]={1,2,1,1};

TComPPS::TComPPS()
: m_PPSId                       (0)
, m_SPSId                       (0)
, m_picInitQPMinus26            (0)
, m_useDQP                      (false)
, m_bConstrainedIntraPred       (false)
, m_pcSPS                       (NULL)
, m_uiMaxCuDQPDepth             (0)
, m_uiMinCuDQPSize              (0)
, m_chromaCbQpOffset            (0)
, m_chromaCrQpOffset            (0)
, m_numRefIdxL0DefaultActive    (1)
, m_numRefIdxL1DefaultActive    (1)
, m_iSliceGranularity           (0)
#if CU_LEVEL_TRANSQUANT_BYPASS
, m_TransquantBypassEnableFlag  (false)
#endif
#if !TILES_OR_ENTROPY_FIX
, m_iTileBehaviorControlPresentFlag (0)
#endif
, m_bLFCrossTileBoundaryFlag     (true)
#if !TILES_OR_ENTROPY_FIX
, m_iColumnRowInfoPresent        (0)
#endif
, m_iUniformSpacingIdr           (0)
, m_iNumColumnsMinus1            (0)
, m_puiColumnWidth               (NULL)
, m_iNumRowsMinus1               (0)
, m_puiRowHeight                 (NULL)
,  m_iNumSubstreams             (1)
, m_signHideFlag(0)
#if !FIXED_SBH_THRESHOLD
, m_signHidingThreshold(0)
#endif
, m_cabacInitPresentFlag        (false)
, m_encCABACTableIdx            (I_SLICE)
#if SLICE_HEADER_EXTENSION
, m_sliceHeaderExtensionPresentFlag    (false)
#endif
{
#if SCALING_LIST_HL_SYNTAX
  m_scalingList = new TComScalingList;
#endif
#if DEPENDENT_SLICES
  m_bDependentSlicesEnabledFlag = false;
  m_bCabacIndependentFlag = false;
#endif
}

TComPPS::~TComPPS()
{
  if( m_iNumColumnsMinus1 > 0 && m_iUniformSpacingIdr == 0 )
  {
    if (m_puiColumnWidth) delete [] m_puiColumnWidth; 
    m_puiColumnWidth = NULL;
  }
  if( m_iNumRowsMinus1 > 0 && m_iUniformSpacingIdr == 0 )
  {
    if (m_puiRowHeight) delete [] m_puiRowHeight;
    m_puiRowHeight = NULL;
  }
#if SCALING_LIST_HL_SYNTAX
  delete m_scalingList;
#endif
}

TComReferencePictureSet::TComReferencePictureSet()
: m_numberOfPictures (0)
, m_numberOfNegativePictures (0)
, m_numberOfPositivePictures (0)
, m_numberOfLongtermPictures (0)
, m_interRPSPrediction (0) 
, m_deltaRIdxMinus1 (0)   
, m_deltaRPS (0) 
, m_numRefIdc (0) 
{
  ::memset( m_deltaPOC, 0, sizeof(m_deltaPOC) );
  ::memset( m_POC, 0, sizeof(m_POC) );
  ::memset( m_used, 0, sizeof(m_used) );
  ::memset( m_refIdc, 0, sizeof(m_refIdc) );
}

TComReferencePictureSet::~TComReferencePictureSet()
{
}

Void TComReferencePictureSet::setUsed(Int bufferNum, Bool used)
{
  m_used[bufferNum] = used;
}

Void TComReferencePictureSet::setDeltaPOC(Int bufferNum, Int deltaPOC)
{
  m_deltaPOC[bufferNum] = deltaPOC;
}

Void TComReferencePictureSet::setNumberOfPictures(Int numberOfPictures)
{
  m_numberOfPictures = numberOfPictures;
}

Int TComReferencePictureSet::getUsed(Int bufferNum)
{
  return m_used[bufferNum];
}

Int TComReferencePictureSet::getDeltaPOC(Int bufferNum)
{
  return m_deltaPOC[bufferNum];
}

Int TComReferencePictureSet::getNumberOfPictures()
{
  return m_numberOfPictures;
}

Int TComReferencePictureSet::getPOC(Int bufferNum)
{
  return m_POC[bufferNum];
}
Void TComReferencePictureSet::setPOC(Int bufferNum, Int POC)
{
  m_POC[bufferNum] = POC;
}
Bool TComReferencePictureSet::getCheckLTMSBPresent(Int bufferNum)
{
  return m_bCheckLTMSB[bufferNum];
}
Void TComReferencePictureSet::setCheckLTMSBPresent(Int bufferNum, Bool b)
{
  m_bCheckLTMSB[bufferNum] = b;
}

/** set the reference idc value at uiBufferNum entry to the value of iRefIdc
 * \param uiBufferNum
 * \param iRefIdc
 * \returns Void
 */
Void TComReferencePictureSet::setRefIdc(Int bufferNum, Int refIdc)
{
  m_refIdc[bufferNum] = refIdc;
}

/** get the reference idc value at uiBufferNum
 * \param uiBufferNum
 * \returns Int
 */
Int  TComReferencePictureSet::getRefIdc(Int bufferNum)
{
  return m_refIdc[bufferNum];
}

/** Sorts the deltaPOC and Used by current values in the RPS based on the deltaPOC values.
 *  deltaPOC values are sorted with -ve values before the +ve values.  -ve values are in decreasing order.
 *  +ve values are in increasing order.
 * \returns Void
 */
Void TComReferencePictureSet::sortDeltaPOC()
{
  // sort in increasing order (smallest first)
  for(Int j=1; j < getNumberOfPictures(); j++)
  { 
    Int deltaPOC = getDeltaPOC(j);
    Bool used = getUsed(j);
    for (Int k=j-1; k >= 0; k--)
    {
      Int temp = getDeltaPOC(k);
      if (deltaPOC < temp)
      {
        setDeltaPOC(k+1, temp);
        setUsed(k+1, getUsed(k));
        setDeltaPOC(k, deltaPOC);
        setUsed(k, used);
      }
    }
  }
  // flip the negative values to largest first
  Int numNegPics = getNumberOfNegativePictures();
  for(Int j=0, k=numNegPics-1; j < numNegPics>>1; j++, k--)
  { 
    Int deltaPOC = getDeltaPOC(j);
    Bool used = getUsed(j);
    setDeltaPOC(j, getDeltaPOC(k));
    setUsed(j, getUsed(k));
    setDeltaPOC(k, deltaPOC);
    setUsed(k, used);
  }
}

/** Prints the deltaPOC and RefIdc (if available) values in the RPS.
 *  A "*" is added to the deltaPOC value if it is Used bu current.
 * \returns Void
 */
Void TComReferencePictureSet::printDeltaPOC()
{
  printf("DeltaPOC = { ");
  for(Int j=0; j < getNumberOfPictures(); j++)
  {
    printf("%d%s ", getDeltaPOC(j), (getUsed(j)==1)?"*":"");
  } 
  if (getInterRPSPrediction()) 
  {
    printf("}, RefIdc = { ");
    for(Int j=0; j < getNumRefIdc(); j++)
    {
      printf("%d ", getRefIdc(j));
    } 
  }
  printf("}\n");
}

TComRPSList::TComRPSList()
:m_referencePictureSets (NULL)
{
}

TComRPSList::~TComRPSList()
{
}

Void TComRPSList::create( Int numberOfReferencePictureSets)
{
  m_numberOfReferencePictureSets = numberOfReferencePictureSets;
  m_referencePictureSets = new TComReferencePictureSet[numberOfReferencePictureSets];
}

Void TComRPSList::destroy()
{
  if (m_referencePictureSets)
  {
    delete [] m_referencePictureSets;
  }
  m_numberOfReferencePictureSets = 0;
  m_referencePictureSets = NULL;
}



TComReferencePictureSet* TComRPSList::getReferencePictureSet(Int referencePictureSetNum)
{
  return &m_referencePictureSets[referencePictureSetNum];
}

Int TComRPSList::getNumberOfReferencePictureSets()
{
  return m_numberOfReferencePictureSets;
}

Void TComRPSList::setNumberOfReferencePictureSets(Int numberOfReferencePictureSets)
{
  m_numberOfReferencePictureSets = numberOfReferencePictureSets;
}

TComRefPicListModification::TComRefPicListModification()
: m_bRefPicListModificationFlagL0 (false)
, m_bRefPicListModificationFlagL1 (false)
{
  ::memset( m_RefPicSetIdxL0, 0, sizeof(m_RefPicSetIdxL0) );
  ::memset( m_RefPicSetIdxL1, 0, sizeof(m_RefPicSetIdxL1) );
}

TComRefPicListModification::~TComRefPicListModification()
{
}

TComAPS::TComAPS()
{
  m_apsID = 0;
#if !AHG6_ALF_OPTION2
  m_bAlfEnabled = false;
#endif
#if !SAO_REMOVE_APS // APS syntax
  m_bSaoEnabled = false;
#endif
  m_pSaoParam = NULL;
#if AHG6_ALF_OPTION2
  m_alfParam[0] = m_alfParam[1] = m_alfParam[2] = NULL;
#else
  m_alfParamSet = NULL;
#endif
#if !SCALING_LIST_HL_SYNTAX
  m_scalingList = NULL;
  m_scalingListEnabled = false;
#endif
}

TComAPS::~TComAPS()
{
  delete m_pSaoParam;
#if AHG6_ALF_OPTION2
  for(Int compIdx =0; compIdx < 3; compIdx++)
  {
    delete m_alfParam[compIdx];
    m_alfParam[compIdx] = NULL;
  }
#else
  delete m_alfParamSet;
#endif
#if !SCALING_LIST_HL_SYNTAX
  delete m_scalingList;
#endif
}

TComAPS& TComAPS::operator= (const TComAPS& src)
{
  m_apsID       = src.m_apsID;
#if !DBL_HL_SYNTAX
  m_loopFilterOffsetInAPS = src.m_loopFilterOffsetInAPS;
  m_loopFilterDisable = src.m_loopFilterDisable;
  m_loopFilterBetaOffsetDiv2 = src.m_loopFilterBetaOffsetDiv2;
  m_loopFilterTcOffsetDiv2 = src.m_loopFilterTcOffsetDiv2;
#endif
#if !AHG6_ALF_OPTION2
  m_bAlfEnabled = src.m_bAlfEnabled;
#endif
#if !SAO_REMOVE_APS // APS syntax
  m_bSaoEnabled = src.m_bSaoEnabled;
#endif
  m_pSaoParam   = src.m_pSaoParam; 
#if AHG6_ALF_OPTION2
  for(Int compIdx =0; compIdx < 3; compIdx++)
  {
    m_alfParam[compIdx] = src.m_alfParam[compIdx];
  }
#else
  m_alfParamSet    = src.m_alfParamSet;
#endif
#if !SCALING_LIST_HL_SYNTAX
  m_scalingList = src.m_scalingList;
  m_scalingListEnabled = src.m_scalingListEnabled;
#endif
#if !SAO_REMOVE_APS // APS syntax
  m_saoInterleavingFlag = src.m_saoInterleavingFlag;
#endif
  return *this;
}

Void TComAPS::createSaoParam()
{
  m_pSaoParam = new SAOParam;
}

Void TComAPS::destroySaoParam()
{
  if(m_pSaoParam != NULL)
  {
    delete m_pSaoParam;
    m_pSaoParam = NULL;
  }
}

Void TComAPS::createAlfParam()
{
#if AHG6_ALF_OPTION2
  for(Int compIdx =0; compIdx < 3; compIdx++)
  {
    m_alfParam[compIdx] = new ALFParam(compIdx);
    m_alfParam[compIdx]->alf_flag = 0;
  }
#else
  m_alfParamSet = new AlfParamSet;
#endif
}
Void TComAPS::destroyAlfParam()
{
#if AHG6_ALF_OPTION2
  for(Int compIdx=0; compIdx < 3; compIdx++)
  {
    if(m_alfParam[compIdx] != NULL)
    {
      delete m_alfParam[compIdx];
      m_alfParam[compIdx] = NULL;
    }
  }
#else
  if(m_alfParamSet != NULL)
  {
    delete m_alfParamSet;
    m_alfParamSet = NULL;
  }
#endif
}

#if !SCALING_LIST_HL_SYNTAX
Void TComAPS::createScalingList()
{
  m_scalingList = new TComScalingList;
}
Void TComAPS::destroyScalingList()
{
  delete m_scalingList;
}
#endif

TComScalingList::TComScalingList()
{
  init();
}
TComScalingList::~TComScalingList()
{
  destroy();
}

/** set default quantization matrix to array
*/
Void TComSlice::setDefaultScalingList()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId=0;listId<g_scalingListNum[sizeId];listId++)
    {
      getScalingList()->processDefaultMarix(sizeId, listId);
    }
  }
}
/** check if use default quantization matrix
 * \returns true if use default quantization matrix in all size
*/
Bool TComSlice::checkDefaultScalingList()
{
  UInt defaultCounter=0;

  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId=0;listId<g_scalingListNum[sizeId];listId++)
    {
#if SCALING_LIST_SIMPLYFY
      if( !memcmp(getScalingList()->getScalingListAddress(sizeId,listId), getScalingList()->getScalingListDefaultAddress(sizeId, listId),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId])) // check value of matrix
     && ((sizeId < SCALING_LIST_16x16) || (getScalingList()->getScalingListDC(sizeId,listId) == 16))) // check DC value
#else
      if(getScalingList()->getUseDefaultScalingMatrixFlag(sizeId,listId))
#endif
      {
        defaultCounter++;
      }
    }
  }
#if SCALING_LIST_HL_SYNTAX
  return (defaultCounter == (SCALING_LIST_NUM * SCALING_LIST_SIZE_NUM - 4)) ? false : true; // -4 for 32x32
#else
  return (defaultCounter == (SCALING_LIST_NUM * SCALING_LIST_SIZE_NUM - 4)) ? true : false; // -4 for 32x32
#endif
}
/** get scaling matrix from RefMatrixID
 * \param sizeId size index
 * \param Index of input matrix
 * \param Index of reference matrix
 */
Void TComScalingList::processRefMatrix( UInt sizeId, UInt listId , UInt refListId )
{
#if SCALING_LIST_SIMPLYFY
  ::memcpy(getScalingListAddress(sizeId, listId),((listId == refListId)? getScalingListDefaultAddress(sizeId, refListId): getScalingListAddress(sizeId, refListId)),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));
#else
  ::memcpy(getScalingListAddress(sizeId, listId),getScalingListAddress(sizeId, refListId),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));
#endif
}
/** parse syntax infomation 
 *  \param pchFile syntax infomation
 *  \returns false if successful
 */
Bool TComScalingList::xParseScalingList(char* pchFile)
{
  FILE *fp;
  Char line[1024];
  UInt sizeIdc,listIdc;
  UInt i,size = 0;
  Int *src=0,data;
  Char *ret;
  UInt  retval;

  if((fp = fopen(pchFile,"r")) == (FILE*)NULL)
  {
    printf("can't open file %s :: set Default Matrix\n",pchFile);
    return true;
  }

  for(sizeIdc = 0; sizeIdc < SCALING_LIST_SIZE_NUM; sizeIdc++)
  {
    size = min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeIdc]);
    for(listIdc = 0; listIdc < g_scalingListNum[sizeIdc]; listIdc++)
    {
      src = getScalingListAddress(sizeIdc, listIdc);

      fseek(fp,0,0);
      do 
      {
        ret = fgets(line, 1024, fp);
        if ((ret==NULL)||(strstr(line, MatrixType[sizeIdc][listIdc])==NULL && feof(fp)))
        {
          printf("Error: can't read Matrix :: set Default Matrix\n");
          return true;
        }
      }
      while (strstr(line, MatrixType[sizeIdc][listIdc]) == NULL);
      for (i=0; i<size; i++)
      {
        retval = fscanf(fp, "%d,", &data);
        if (retval!=1)
        {
          printf("Error: can't read Matrix :: set Default Matrix\n");
          return true;
        }
        src[i] = data;
      }
      //set DC value for default matrix check
      setScalingListDC(sizeIdc,listIdc,src[0]);

      if(sizeIdc > SCALING_LIST_8x8)
      {
        fseek(fp,0,0);
        do 
        {
          ret = fgets(line, 1024, fp);
          if ((ret==NULL)||(strstr(line, MatrixType_DC[sizeIdc][listIdc])==NULL && feof(fp)))
          {
            printf("Error: can't read DC :: set Default Matrix\n");
            return true;
          }
        }
        while (strstr(line, MatrixType_DC[sizeIdc][listIdc]) == NULL);
          retval = fscanf(fp, "%d,", &data);
          if (retval!=1)
          {
            printf("Error: can't read Matrix :: set Default Matrix\n");
            return true;
          }
          //overwrite DC value when size of matrix is larger than 16x16
          setScalingListDC(sizeIdc,listIdc,data);
      }
    }
  }
  fclose(fp);
  return false;
}

/** initialization process of quantization matrix array
 */
Void TComScalingList::init()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
    {
      m_scalingListCoef[sizeId][listId] = new Int [min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId])];
    }
  }
  m_scalingListCoef[SCALING_LIST_32x32][3] = m_scalingListCoef[SCALING_LIST_32x32][1]; // copy address for 32x32
}
/** destroy quantization matrix array
 */
Void TComScalingList::destroy()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
    {
      if(m_scalingListCoef[sizeId][listId]) delete [] m_scalingListCoef[sizeId][listId];
    }
  }
}
/** get default address of quantization matrix 
 * \param sizeId size index
 * \param listId list index
 * \returns pointer of quantization matrix
 */
Int* TComScalingList::getScalingListDefaultAddress(UInt sizeId, UInt listId)
{
  Int *src = 0;
  switch(sizeId)
  {
    case SCALING_LIST_4x4:
      src = (listId<3) ? g_quantIntraDefault4x4 : g_quantInterDefault4x4;
      break;
    case SCALING_LIST_8x8:
      src = (listId<3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
      break;
    case SCALING_LIST_16x16:
      src = (listId<3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
      break;
    case SCALING_LIST_32x32:
      src = (listId<1) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
      break;
    default:
      assert(0);
      src = NULL;
      break;
  }
  return src;
}
/** process of default matrix
 * \param sizeId size index
 * \param Index of input matrix
 */
Void TComScalingList::processDefaultMarix(UInt sizeId, UInt listId)
{
  ::memcpy(getScalingListAddress(sizeId, listId),getScalingListDefaultAddress(sizeId,listId),sizeof(Int)*min(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));
#if !SCALING_LIST_SIMPLYFY
  setUseDefaultScalingMatrixFlag(sizeId,listId,true);
#endif
  setScalingListDC(sizeId,listId,SCALING_LIST_DC);
}
/** check DC value of matrix for default matrix signaling
 */
Void TComScalingList::checkDcOfMatrix()
{
  for(UInt sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
  {
    for(UInt listId = 0; listId < g_scalingListNum[sizeId]; listId++)
    {
#if !SCALING_LIST_SIMPLYFY
      setUseDefaultScalingMatrixFlag(sizeId,listId,false);
#endif
      //check default matrix?
      if(getScalingListDC(sizeId,listId) == 0)
      {
        processDefaultMarix(sizeId, listId);
      }
    }
  }
}

ParameterSetManager::ParameterSetManager()
#if VPS_INTEGRATION 
: m_vpsMap(MAX_NUM_VPS)
, m_spsMap(MAX_NUM_SPS)
#else
: m_spsMap(MAX_NUM_SPS)
#endif
, m_ppsMap(MAX_NUM_PPS)
, m_apsMap(MAX_NUM_APS)
{
}


ParameterSetManager::~ParameterSetManager()
{
}

//! \}
