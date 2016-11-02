// @(#)root/graf:$Id$
// Author: Georg Troska 19/05/16

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <stdlib.h>
#include <iostream>
#include "TROOT.h"
#include "TCandle.h"
#include "TClass.h"
#include "TPad.h"
#include "TRandom2.h"

ClassImp(TCandle)

/** \class TCandle
\ingroup BasicGraphics

The candle plot painter class.

Instances of this class are generated by the histograms painting
classes (THistPainter and THStack) when an candle plot (box plot) is drawn.
TCandle is the "painter class" of the box plots. Therefore it is never used
directly to draw a candle.
*/



////////////////////////////////////////////////////////////////////////////////
/// TCandle default constructor.

TCandle::TCandle()
{
   fIsCalculated  = 0;
   fIsRaw         = 0;
   fPosCandleAxis = 0.;
   fCandleWidth   = 1.0;
   fMean          = 0.;
   fMedian        = 0.;
   fMedianErr     = 0;
   fBoxUp         = 0.;
   fBoxDown       = 0.;
   fWhiskerUp     = 0.;
   fWhiskerDown   = 0.;
   fNDatapoints   = 0;
   fDismiss = 0;
   fLogX          = 0;
   fLogY          = 0;
   fNDrawPoints     = 0;
   fNHistoPoints  = 0;
}

////////////////////////////////////////////////////////////////////////////////
/// TCandle constructor for raw-data candles.

TCandle::TCandle(const Double_t candlePos, const Double_t candleWidth, Long64_t n, Double_t * points) 
   : TAttLine(), TAttFill(), TAttMarker()
{
   //Preliminary values only, need to be calculated before paint
   fMean          = 0;
   fMedian        = 0;
   fMedianErr     = 0;
   fBoxUp         = 0;
   fBoxDown       = 0;
   fWhiskerUp     = 0;
   fWhiskerDown   = 0;
   fNDatapoints   = n;
   fIsCalculated  = 0;
   fIsRaw         = true;
   fPosCandleAxis = candlePos;
   fCandleWidth   = candleWidth;
   fDatapoints    = points;
   fProj          = NULL;
   fDismiss       = 0;
   fOption        = kNoOption;
   fLogX          = 0;
   fLogY          = 0;
   fNDrawPoints     = 0;
   fNHistoPoints  = 0;
   
}

////////////////////////////////////////////////////////////////////////////////
/// TCandle TH1 data constructor.

TCandle::TCandle(const Double_t candlePos, const Double_t candleWidth, TH1D *proj)
   : TAttLine(), TAttFill(), TAttMarker()
{
   //Preliminary values only, need to be calculated before paint
   fMean          = 0;
   fMedian        = 0;
   fMedianErr     = 0;
   fBoxUp         = 0;
   fBoxDown       = 0;
   fWhiskerUp     = 0;
   fWhiskerDown   = 0;
   fNDatapoints   = 0;
   fIsCalculated  = 0;
   fIsRaw         = 0;
   fPosCandleAxis = candlePos;
   fCandleWidth   = candleWidth;
   fDatapoints    = 0;
   fProj          = proj;
   fDismiss       = 0;
   fOption        = kNoOption;
   fLogX          = 0;
   fLogY          = 0;
   fNDrawPoints     = 0;
   fNHistoPoints  = 0;

}

////////////////////////////////////////////////////////////////////////////////
/// TCandle default destructor.

TCandle::~TCandle() {
   if (fIsRaw && fProj) delete fProj;
}

////////////////////////////////////////////////////////////////////////////////
/// Parsing of the option-string.
/// The option-string will be empty at the end (by-reference).

int TCandle::ParseOption(char * opt) {
   fOption = kNoOption;
   char *l;
   
   l = strstr(opt,"CANDLE");
   if (l) {
      const CandleOption fallbackCandle = (CandleOption)(kBox + kMedianLine + kMeanCircle + kWhiskerAll + kAnchor);

      char direction = ' ';
      char preset = ' ';

      if (l[6] >= 'A' && l[6] <= 'Z') direction = l[6];
      if (l[6] >= '1' && l[6] <= '9') preset = l[6];
      if (l[7] >= 'A' && l[7] <= 'Z' && preset != ' ') direction = l[7];
      if (l[7] >= '1' && l[7] <= '9' && direction != ' ') preset = l[7];

      if (direction == 'X' || direction == 'V') { /* nothing */ }
      if (direction == 'Y' || direction == 'H') { fOption = (CandleOption)(fOption + kHorizontal); }
      if (preset == '1') //Standard candle using old candle-definition
         fOption = (CandleOption)(fOption + fallbackCandle);
      if (preset == '2') //New standard candle with better whisker definition + outlier
         fOption = (CandleOption)(fOption + kBox + kMeanLine + kMedianLine + kWhisker15 + kAnchor + kPointsOutliers);
      if (preset == '3')  //Like candle2 but with a fMean as a circle
         fOption = (CandleOption)(fOption + kBox + kMeanCircle + kMedianLine + kWhisker15 + kAnchor + kPointsOutliers);
      if (preset == '4')  //Like candle3 but showing the uncertainty of the fMedian as well
         fOption = (CandleOption)(fOption + kBox + kMeanCircle + kMedianNotched + kWhisker15 + kAnchor + kPointsOutliers);
      if (preset == '5')  //Like candle2 but showing all datapoints
         fOption = (CandleOption)(fOption + kBox + kMeanLine + kMedianLine + kWhisker15 + kAnchor + kPointsAll);
      if (preset == '6')  //Like candle2 but showing all datapoints scattered
         fOption = (CandleOption)(fOption + kBox + kMeanCircle + kMedianLine + kWhisker15 + kAnchor + kPointsAllScat);

      if (preset != ' ' && direction != ' ')
         strncpy(l,"        ",8);
      else if (preset != ' ' || direction != ' ')
         strncpy(l,"        ",7);
      else
         strncpy(l,"        ",6);

      Bool_t useIndivOption = false;

      if (preset == ' ') { // Check if the user wants to set the properties individually
         char *brOpen = strstr(opt,"(");
         char *brClose = strstr(opt,")");
         char indivOption[32];
         if (brOpen && brClose) {
            useIndivOption = true;
            bool isHorizontal = IsHorizontal();
            strncpy(indivOption, brOpen, brClose-brOpen +1); //Now the string "(....)" including brackets is in this array
            sscanf(indivOption,"(%d)", (int*) &fOption);
            if (isHorizontal) {fOption = (CandleOption)(fOption + kHorizontal);}
            strncpy(brOpen,"                ",brClose-brOpen+1); //Cleanup

         }
      }
      //Handle option "CANDLE" ,"CANDLEX" or "CANDLEY" to behave like "CANDLEX1" or "CANDLEY1"
      if (!useIndivOption && !fOption ) {
         fOption = fallbackCandle;
      }
   }
   fIsCalculated = false;
   return fOption;

}

////////////////////////////////////////////////////////////////////////////////
/// Calculates all values needed by the candle definition depending on the
/// candle options.

void TCandle::Calculate() {
   
   //Reset everything
   fNDrawPoints = 0;
   fNHistoPoints = 0;
   
   Bool_t swapXY = IsOption(kHorizontal);
   Bool_t doLogY = (!(swapXY) && fLogY) || (swapXY && fLogX);
   Bool_t doLogX = (!(swapXY) && fLogX) || (swapXY && fLogY);
   
   //Will be min and max values of raw-data
   Double_t min = 1e15;
   Double_t max = -1e15;
  
   // Determining the quantiles
   Double_t *prob = new Double_t[5];
   prob[0]=1E-15; prob[1]=0.25; prob[2]=0.5; prob[3]=0.75; prob[4]=1-1E-15;
   Double_t *quantiles = new Double_t[5];
   quantiles[0]=0.; quantiles[1]=0.; quantiles[2] = 0.; quantiles[3] = 0.; quantiles[4] = 0.;
   if (!fIsRaw && fProj) { //Need a calculation for a projected histo
      fProj->GetQuantiles(5, quantiles, prob);
   } else { //Need a calculation for a raw-data candle
      TMath::Quantiles(fNDatapoints,5,fDatapoints,quantiles,prob,kFALSE);
   }

   // Check if the quantiles are valid, seems the under- and overflow is taken
   // into account as well, we need to ignore this!
   if (quantiles[0] >= quantiles[4]) return;
   if (quantiles[1] >= quantiles[3]) return;

   // Definition of the candle in the standard case
   fBoxUp = quantiles[3];
   fBoxDown = quantiles[1];
   fWhiskerUp = quantiles[4]; //Standard case
   fWhiskerDown = quantiles[0]; //Standard case
   fMedian = quantiles[2];
   Double_t iqr = fBoxUp-fBoxDown;
   
   if (!fIsRaw && fProj) { //Need a calculation for a projected histo
      fMean = fProj->GetMean();
      fMedianErr = 1.57*iqr/sqrt(fProj->GetEntries());
      fAxisMin = fProj->GetXaxis()->GetXmin();
      fAxisMax = fProj->GetXaxis()->GetXmax();
    } else { //Need a calculation for a raw-data candle
      //Calculate the Mean
      fMean = 0;
      for (Long64_t i = 0; i < fNDatapoints; ++i) {
       fMean += fDatapoints[i];
       if (fDatapoints[i] < min) min = fDatapoints[i];
       if (fDatapoints[i] > max) max = fDatapoints[i];
      }
      fMean /= fNDatapoints;
      fMedianErr = 1.57*iqr/sqrt(fNDatapoints);
    }

   if (IsOption(kWhisker15)) { // Improved whisker definition, with 1.5*iqr
      if (!fIsRaw && fProj) { //Need a calculation for a projected histo
       int bin = fProj->FindBin(fBoxDown-1.5*iqr);
       // extending only to the lowest data value within this range
       while (fProj->GetBinContent(bin) == 0 && bin <= fProj->GetNbinsX()) bin++;
       fWhiskerDown = fProj->GetBinCenter(bin);

       bin = fProj->FindBin(fBoxUp+1.5*iqr);
       while (fProj->GetBinContent(bin) == 0 && bin >= 1) bin--;
       fWhiskerUp = fProj->GetBinCenter(bin);
      } else { //Need a calculation for a raw-data candle
       fWhiskerUp = fBoxDown;
       fWhiskerDown = fBoxUp;

       //Need to find highest value up to 1.5*iqr from the BoxUp-pos, and the lowest value up to -1.5*iqr from the boxLow-pos
       for (Long64_t i = 0; i < fNDatapoints; ++i) {
         Double_t myData = fDatapoints[i];
         if (myData > fWhiskerUp && myData <= fBoxUp + 1.5*iqr) fWhiskerUp = myData;
         if (myData < fWhiskerDown && myData >= fBoxDown - 1.5*iqr) fWhiskerDown = myData;
       }
    
      }
   }

   delete [] prob;
   delete [] quantiles;
   
   
   //Doing the outliers and other single points to show
    if (GetCandleOption(5) > 0) { //Draw outliers
      TRandom2 random;
      const int maxOutliers = kNMAXPOINTS;
      Double_t myScale = 1.;
      if (!fIsRaw && fProj) { //Need a calculation for a projected histo
    if (fProj->GetEntries() > maxOutliers/2) myScale = fProj->GetEntries()/(maxOutliers/2.);
    fNDrawPoints = 0;
    for (int bin = 0; bin < fProj->GetNbinsX(); bin++) {
       // Either show them only outside the whiskers, or all of them
       if (fProj->GetBinContent(bin) > 0 && (fProj->GetBinCenter(bin) < fWhiskerDown || fProj->GetBinCenter(bin) > fWhiskerUp || (GetCandleOption(5) > 1)) ) {
       Double_t scaledBinContent = fProj->GetBinContent(bin)/myScale;
       if (scaledBinContent >0 && scaledBinContent < 1) scaledBinContent = 1; //Outliers have a typical bincontent between 0 and 1, when scaling they would disappear
          for (int j=0; j < (int)scaledBinContent; j++) {
        if (fNDrawPoints > maxOutliers) break;
        if (IsOption(kPointsAllScat)) { //Draw outliers and "all" values scattered
           fDrawPointsX[fNDrawPoints] = fPosCandleAxis - fCandleWidth/2. + fCandleWidth*random.Rndm();
           fDrawPointsY[fNDrawPoints] = fProj->GetBinLowEdge(bin) + fProj->GetBinWidth(bin)*random.Rndm();
        } else { //Draw them in the "candle line"
           fDrawPointsX[fNDrawPoints] = fPosCandleAxis;
           if ((int)scaledBinContent == 1) //If there is only one datapoint available put it in the middle of the bin
         fDrawPointsY[fNDrawPoints] = fProj->GetBinCenter(bin);
           else //If there is more than one datapoint scatter it along the bin, otherwise all marker would be (invisibly) stacked on top of each other
         fDrawPointsY[fNDrawPoints] = fProj->GetBinLowEdge(bin) + fProj->GetBinWidth(bin)*random.Rndm();
        }
        if (swapXY) {
           //Swap X and Y
           Double_t keepCurrently;
           keepCurrently = fDrawPointsX[fNDrawPoints];
           fDrawPointsX[fNDrawPoints] = fDrawPointsY[fNDrawPoints];
           fDrawPointsY[fNDrawPoints] = keepCurrently;
        }
        // Continue fMeans, that fNDrawPoints is not increased, so that value will not be shown
        if (doLogX) {
           if (fDrawPointsX[fNDrawPoints] > 0) fDrawPointsX[fNDrawPoints] = TMath::Log10(fDrawPointsX[fNDrawPoints]); else continue;
        }
        if (doLogY) {
           if (fDrawPointsY[fNDrawPoints] > 0) fDrawPointsY[fNDrawPoints] = TMath::Log10(fDrawPointsY[fNDrawPoints]); else continue;
        }
        fNDrawPoints++;
          }
       }
       if (fNDrawPoints > maxOutliers) { //Should never happen, due to myScale!!!
          Error ("PaintCandlePlot","Not possible to draw all outliers.");
          break;
       }
    }
      } else { //Raw data candle
    if (fNDatapoints > maxOutliers/2) myScale = fNDatapoints/(maxOutliers/2.);
    fNDrawPoints = 0;
    for (int i = 0; i < fNDatapoints; i++ ) { 
       Double_t myData = fDatapoints[i];
       Double_t maxScatter = (fWhiskerUp-fWhiskerDown)/100;
       if (!(i % (int) myScale == 0 || myData < fWhiskerDown || myData > fWhiskerUp )) continue; //If the amount of data is too large take only every 2nd or 3rd to reduce the amount, but do not reduce at outliers!
       // Either show them only outside the whiskers, or all of them
       if (myData < fWhiskerDown || myData > fWhiskerUp || (GetCandleOption(5) > 1)) {
          if (IsOption(kPointsAllScat)) { //Draw outliers and "all" values scattered
        fDrawPointsX[fNDrawPoints] = fPosCandleAxis - fCandleWidth/2. + fCandleWidth*random.Rndm();
        fDrawPointsY[fNDrawPoints] = myData + (random.Rndm() - 0.5)*maxScatter; //random +- 0.5 of candleheight
          } else { //Draw them in the "candle line"
        fDrawPointsX[fNDrawPoints] = fPosCandleAxis; 
        fDrawPointsY[fNDrawPoints] = myData + (random.Rndm() - 0.5)*maxScatter; //random +- 0.5 of candleheight
          }
          if (swapXY) {
        //Swap X and Y
        Double_t keepCurrently;
        keepCurrently = fDrawPointsX[fNDrawPoints];
        fDrawPointsX[fNDrawPoints] = fDrawPointsY[fNDrawPoints];
        fDrawPointsY[fNDrawPoints] = keepCurrently;
          }
          // Continue fMeans, that fNDrawPoints is not increased, so that value will not be shown
          if (doLogX) {
        if (fDrawPointsX[fNDrawPoints] > 0) fDrawPointsX[fNDrawPoints] = TMath::Log10(fDrawPointsX[fNDrawPoints]); else continue;
          }
          if (doLogY) {
        if (fDrawPointsY[fNDrawPoints] > 0) fDrawPointsY[fNDrawPoints] = TMath::Log10(fDrawPointsY[fNDrawPoints]); else continue;
          }
       }
       fNDrawPoints++;
       if (fNDrawPoints > maxOutliers) { //Should never happen, due to myScale!!!
          Error ("PaintCandlePlot","Not possible to draw all outliers.");
          break;
       }
      
    }
    
      }
       
   }
   
   if (IsOption(kHistoRight) || IsOption(kHistoLeft) || IsOption(kHistoViolin)) {
      //We are starting with kHistoRight, left will be modified from right later
      if (fIsRaw) { //This is a raw-data candle
         if (!fProj) {
            fProj = new TH1D("hpa","hpa",100,min,max+0.0001*(max-min));
            for (Long64_t i = 0; i < fNDatapoints; ++i) {
               fProj->Fill(fDatapoints[i]);
            }
         }
      }
    //   if (!fIsRaw && fProj) { //Need a calculation for a projected histo
     fNHistoPoints = 0;
     Double_t maxContent = fProj->GetMaximum();
     Double_t maxHistoHeight = fCandleWidth*0.8;
    // fHistoPointsX[fNHistoPoints] = fPosCandleAxis;
     //fHistoPointsY[fNHistoPoints] = fProj->GetXaxis()->GetXmin();
     bool isFirst = true;
     int lastNonZero = 0;
      for (int bin = 1; bin <= fProj->GetNbinsX(); bin++) {
         if (isFirst) {
            if (fProj->GetBinContent(bin) > 0) {
               fHistoPointsX[fNHistoPoints] = fPosCandleAxis;
               fHistoPointsY[fNHistoPoints] = fProj->GetBinLowEdge(bin);
                if (doLogX) {
                  if (fHistoPointsX[fNHistoPoints] > 0) fHistoPointsX[fNHistoPoints] = TMath::Log10(fHistoPointsX[fNHistoPoints]); else continue;
               }
               if (doLogY) {
                  if (fHistoPointsY[fNHistoPoints] > 0) fHistoPointsY[fNHistoPoints] = TMath::Log10(fHistoPointsY[fNHistoPoints]); else continue;
               }
               fNHistoPoints++;
               isFirst = false;
            } else {
               continue;
            }
         }
         Double_t myBinValue = fProj->GetBinContent(bin);
         fHistoPointsX[fNHistoPoints] = fPosCandleAxis + myBinValue/maxContent*maxHistoHeight;
         fHistoPointsY[fNHistoPoints] = fProj->GetBinLowEdge(bin);
         fNHistoPoints++;
         fHistoPointsX[fNHistoPoints] = fPosCandleAxis + myBinValue/maxContent*maxHistoHeight;
         fHistoPointsY[fNHistoPoints] = fProj->GetBinLowEdge(bin)+fProj->GetBinWidth(bin);
          if (doLogX) {
            if (fHistoPointsX[fNHistoPoints -1] > 0) fHistoPointsX[fNHistoPoints - 1] = TMath::Log10(fHistoPointsX[fNHistoPoints - 1]); else continue;
            if (fHistoPointsX[fNHistoPoints] > 0) fHistoPointsX[fNHistoPoints] = TMath::Log10(fHistoPointsX[fNHistoPoints]); else continue;
          }
          if (doLogY) {
            if (fHistoPointsY[fNHistoPoints -1] > 0) fHistoPointsY[fNHistoPoints - 1] = TMath::Log10(fHistoPointsY[fNHistoPoints - 1]); else continue;
            if (fHistoPointsY[fNHistoPoints] > 0) fHistoPointsY[fNHistoPoints] = TMath::Log10(fHistoPointsY[fNHistoPoints]); else continue;
          }
         
         fNHistoPoints++;
         if (fProj->GetBinContent(bin) > 0) lastNonZero = fNHistoPoints;
      }
      
      fHistoPointsX[fNHistoPoints] = fPosCandleAxis;
      fHistoPointsY[fNHistoPoints] = fHistoPointsY[fNHistoPoints-1];
      fNHistoPoints = lastNonZero+1; //+1 so that the line down to 0 is added as well
      /*
      fHistoPointsX[fNHistoPoints] = fHistoPointsX[0];
      fHistoPointsY[fNHistoPoints] = fHistoPointsY[0];
      fNHistoPoints++;
      */
      if (IsOption(kHistoLeft)) {
         for (int i = 0; i < fNHistoPoints; i++) {
            fHistoPointsX[i] = 2*fPosCandleAxis - fHistoPointsX[i];
         }
      }
      if (IsOption(kHistoViolin)) {
         for (int i = 0; i < fNHistoPoints; i++) {
            fHistoPointsX[fNHistoPoints + i] = 2*fPosCandleAxis - fHistoPointsX[fNHistoPoints -i-1];
            fHistoPointsY[fNHistoPoints + i] = fHistoPointsY[fNHistoPoints -i-1];
         }
         fNHistoPoints *= 2;
         //fNHistoPoints -= 2;
      }
      /* } else { //Raw histo
         std::cout << "Calculation of raw histo is still missing! " << std::endl;
       }*/
   }
   
   fIsCalculated = true;
}

////////////////////////////////////////////////////////////////////////////////
/// Paint one candle with its current attributes.

void TCandle::Paint(Option_t *)
{
  //If something was changed before, we need to recalculate some values
   if (!fIsCalculated) Calculate();

   // Save the attributes as they were set originally
   Style_t saveLine   = GetLineStyle();
   Style_t saveMarker = GetMarkerStyle();
   Style_t saveFillStyle = GetFillStyle();
   Style_t saveFillColor = GetFillColor();
   Style_t saveLineColor = GetLineColor();

   Double_t dimLeft = fPosCandleAxis-0.5*fCandleWidth;
   Double_t dimRight = fPosCandleAxis+0.5*fCandleWidth;
   
 

   TAttLine::Modify();
   TAttFill::Modify();
   TAttMarker::Modify();

   Bool_t swapXY = IsOption(kHorizontal);
   Bool_t doLogY = (!(swapXY) && fLogY) || (swapXY && fLogX);
   Bool_t doLogX = (!(swapXY) && fLogX) || (swapXY && fLogY);

   // From now on this is real painting only, no calculations anymore
   
   if (IsOption(kHistoZeroIndicator)) {
      SetLineColor(saveFillColor);
      TAttLine::Modify();
      PaintLine(fPosCandleAxis, fAxisMin, fPosCandleAxis, fAxisMax, swapXY);
      SetLineColor(saveLineColor);
      TAttLine::Modify();
   }

   if (IsOption(kHistoRight) || IsOption(kHistoLeft) || IsOption(kHistoViolin)) {
      if (IsOption(kHistoZeroIndicator)) {
         SetLineColor(saveFillColor);
         TAttLine::Modify();
      }
      if (!swapXY) {
         gPad->PaintFillArea(fNHistoPoints, fHistoPointsX, fHistoPointsY);
         gPad->PaintPolyLine(fNHistoPoints, fHistoPointsX, fHistoPointsY);
      } else {
         gPad->PaintFillArea(fNHistoPoints, fHistoPointsY, fHistoPointsX);
         gPad->PaintPolyLine(fNHistoPoints, fHistoPointsY, fHistoPointsX);
      }
      if (IsOption(kHistoZeroIndicator)) {
         SetLineColor(saveLineColor);
         TAttLine::Modify();
      }
   }
   

   if (IsOption(kBox)) { // Draw a simple box
     if (IsOption(kMedianNotched)) { // Check if we have to draw a box with notches
         Double_t x[] = {dimLeft,  dimLeft, dimLeft+fCandleWidth/3., dimLeft, dimLeft, dimRight,
                         dimRight, dimRight-fCandleWidth/3., dimRight, dimRight, dimLeft};
         Double_t y[] = {fBoxDown, fMedian-fMedianErr, fMedian, fMedian+fMedianErr, fBoxUp, fBoxUp,
                         fMedian+fMedianErr, fMedian, fMedian-fMedianErr, fBoxDown, fBoxDown};
         PaintBox(11, x, y, swapXY);
      } else { // draw a simple box
         Double_t x[] = {dimLeft, dimLeft, dimRight, dimRight, dimLeft};
         Double_t y[] = {fBoxDown,  fBoxUp, fBoxUp,  fBoxDown,   fBoxDown};
         PaintBox(5, x, y, swapXY);
      }
   } 

   if (IsOption(kAnchor)) { // Draw the anchor line
      PaintLine(dimLeft, fWhiskerUp, dimRight, fWhiskerUp, swapXY);
      PaintLine(dimLeft, fWhiskerDown, dimRight, fWhiskerDown, swapXY);
   }

   if (IsOption(kWhiskerAll) && !IsOption(kHistoZeroIndicator)) { // Whiskers are dashed
      SetLineStyle(2);
      TAttLine::Modify();
      PaintLine(fPosCandleAxis, fWhiskerUp, fPosCandleAxis, fBoxUp, swapXY);
      PaintLine(fPosCandleAxis, fBoxDown, fPosCandleAxis, fWhiskerDown, swapXY);
      SetLineStyle(saveLine);
      TAttLine::Modify();
   }  else if ((IsOption(kWhiskerAll) && IsOption(kHistoZeroIndicator)) || IsOption(kWhisker15) ) { // Whiskers without dashing, better whisker definition, or forced when using zero line
      PaintLine(fPosCandleAxis, fWhiskerUp, fPosCandleAxis, fBoxUp, swapXY);
      PaintLine(fPosCandleAxis, fBoxDown, fPosCandleAxis, fWhiskerDown, swapXY);
   } 

   if (IsOption(kMedianLine)) { // Paint fMedian as a line
      PaintLine(dimLeft, fMedian, dimRight, fMedian, swapXY);
   } else if (IsOption(kMedianNotched)) { // Paint fMedian as a line (using notches, fMedian line is shorter)
      PaintLine(dimLeft+fCandleWidth/3, fMedian, dimRight-fCandleWidth/3., fMedian, swapXY);
   } else if (IsOption(kMedianCircle)) { // Paint fMedian circle
      Double_t myMedianX[1], myMedianY[1];
      if (!swapXY) {
         myMedianX[0] = fPosCandleAxis;
         myMedianY[0] = fMedian;
      } else {
         myMedianX[0] = fMedian;
         myMedianY[0] = fPosCandleAxis;
      }

      Bool_t isValid = true;
      if (doLogX) {
         if (myMedianX[0] > 0) myMedianX[0] = TMath::Log10(myMedianX[0]); else isValid = false;
      }
      if (doLogY) {
         if (myMedianY[0] > 0) myMedianY[0] = TMath::Log10(myMedianY[0]); else isValid = false;
      }

      SetMarkerStyle(24);
      TAttMarker::Modify();

      if (isValid) gPad->PaintPolyMarker(1,myMedianX,myMedianY); // A circle for the fMedian

      SetMarkerStyle(saveMarker);
      TAttMarker::Modify();

   }
  if (IsOption(kMeanCircle)) { // Paint fMean as a circle
      Double_t myMeanX[1], myMeanY[1];
      if (!swapXY) {
         myMeanX[0] = fPosCandleAxis;
         myMeanY[0] = fMean;
      } else {
         myMeanX[0] = fMean;
         myMeanY[0] = fPosCandleAxis;
      }

      Bool_t isValid = true;
      if (doLogX) {
         if (myMeanX[0] > 0) myMeanX[0] = TMath::Log10(myMeanX[0]); else isValid = false;
      }
      if (doLogY) {
         if (myMeanY[0] > 0) myMeanY[0] = TMath::Log10(myMeanY[0]); else isValid = false;
      }

      SetMarkerStyle(24);
      TAttMarker::Modify();

      if (isValid) gPad->PaintPolyMarker(1,myMeanX,myMeanY); // A circle for the fMean

      SetMarkerStyle(saveMarker);
      TAttMarker::Modify();

   } else if (IsOption(kMeanLine)) { // Paint fMean as a dashed line
      SetLineStyle(2);
      TAttLine::Modify();

      PaintLine(dimLeft, fMean, dimRight, fMean, swapXY);
      SetLineStyle(saveLine);
      TAttLine::Modify();

   }

   if (IsOption(kAnchor)) { //Draw standard anchor
      PaintLine(dimLeft, fWhiskerDown, dimRight, fWhiskerDown, swapXY); // the lower anchor line
      PaintLine(dimLeft, fWhiskerUp, dimRight, fWhiskerUp, swapXY); // the upper anchor line
   }

   // This is a bit complex. All values here are handled as outliers. Usually
   // only the datapoints outside the whiskers are shown.
   // One can show them in one row as crosses, or scattered randomly. If activated
   // all datapoint are shown in the same way
   
   if (GetCandleOption(5) > 0) { //Draw outliers
     if (IsOption(kPointsAllScat)) { //Draw outliers and "all" values scattered
    SetMarkerStyle(0);
      } else {
    SetMarkerStyle(5);
      }
      TAttMarker::Modify();
      gPad->PaintPolyMarker(fNDrawPoints,fDrawPointsX, fDrawPointsY);
      
   }
   

}

////////////////////////////////////////////////////////////////////////////////
/// Return true is this option is activated in fOption

bool TCandle::IsOption(CandleOption opt) {
   long myOpt = 9;
   int pos = 0;
   for (pos = 0; pos < 16; pos++) {
      if (myOpt > opt) break;
      else myOpt *=10;
   }
   myOpt /= 9;
   int thisOpt = GetCandleOption(pos);

   return ((thisOpt * myOpt) == opt);
}

////////////////////////////////////////////////////////////////////////////////
/// Paint a box for candle.

void TCandle::PaintBox(Int_t nPoints, Double_t *x, Double_t *y, Bool_t swapXY)
{
   Bool_t doLogY = (!(swapXY) && fLogY) || (swapXY && fLogX);
   Bool_t doLogX = (!(swapXY) && fLogX) || (swapXY && fLogY);
   if (doLogY) {
      for (int i=0; i<nPoints; i++) {
         if (y[i] > 0) y[i] = TMath::Log10(y[i]);
         else return;
      }
   }
   if (doLogX) {
      for (int i=0; i<nPoints; i++) {
         if (x[i] > 0) x[i] = TMath::Log10(x[i]);
         else return;
      }
   }
   if (!swapXY) {
     gPad->PaintFillArea(nPoints, x, y);

      gPad->PaintPolyLine(nPoints, x, y);
   } else {
      gPad->PaintFillArea(nPoints, y, x);
      gPad->PaintPolyLine(nPoints, y, x);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Paint a line for candle.

void TCandle::PaintLine(Double_t x1, Double_t y1, Double_t x2, Double_t y2, Bool_t swapXY)
{
   Bool_t doLogY = (!(swapXY) && fLogY) || (swapXY && fLogX);
   Bool_t doLogX = (!(swapXY) && fLogX) || (swapXY && fLogY);
   if (doLogY) {
      if (y1 > 0) y1 = TMath::Log10(y1); else return;
      if (y2 > 0) y2 = TMath::Log10(y2); else return;
   }
   if (doLogX) {
      if (x1 > 0) x1 = TMath::Log10(x1); else return;
      if (x2 > 0) x2 = TMath::Log10(x2); else return;
   }
   if (!swapXY) {
      gPad->PaintLine(x1, y1, x2, y2);
   } else {
      gPad->PaintLine(y1, x1, y2, x2);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Stream an object of class TCandle.

void TCandle::Streamer(TBuffer &R__b)
{
   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 3) {
         R__b.ReadClassBuffer(TCandle::Class(), this, R__v, R__s, R__c);
         return;
      }
   } else {
      R__b.WriteClassBuffer(TCandle::Class(),this);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// The coordinates in the TParallelCoordVar-class are in Pad-Coordinates, so we need to convert them

void TCandle::ConvertToPadCoords(Double_t minAxis, Double_t maxAxis, Double_t axisMinCoord, Double_t axisMaxCoord, Double_t fMinInit, Double_t fMaxInit) {
   
   /* THIS IS UGLY - WE SHOULD DEFINE A BETTER COORDINATE SYSTEM IN THE PAD */
   
    if (!fIsCalculated) Calculate();
   Double_t a,b,maxinit,mininit;
   if (fLogY) {
      a = TMath::Log10(minAxis);
      b = TMath::Log10(maxAxis/minAxis);
      if(fMinInit > 0) mininit = TMath::Log10(fMinInit);
      else             mininit = TMath::Log10(axisMinCoord);
      maxinit = TMath::Log10(fMaxInit);
   } else {
      a = minAxis;
      b = maxAxis-minAxis;
      mininit = fMinInit;
      maxinit = fMaxInit;
   }
   
   std::cout << "DOING UGLY CONVERSION! PLEASE FIXME!!!" << std::endl;
   fMean = axisMinCoord + ((fMean-a)/b)*(axisMaxCoord-axisMinCoord);
   fMedian = axisMinCoord + ((fMedian-a)/b)*(axisMaxCoord-axisMinCoord);
   fMedianErr  = axisMinCoord + ((fMedianErr-a)/b)*(axisMaxCoord-axisMinCoord);
   fBoxUp  = axisMinCoord + ((fBoxUp-a)/b)*(axisMaxCoord-axisMinCoord);
   fBoxDown  = axisMinCoord + ((fBoxDown-a)/b)*(axisMaxCoord-axisMinCoord);
   fWhiskerUp  = axisMinCoord + ((fWhiskerUp-a)/b)*(axisMaxCoord-axisMinCoord);
   fWhiskerDown  = axisMinCoord + ((fWhiskerDown-a)/b)*(axisMaxCoord-axisMinCoord);
   
   for (int i = 0; i < fNDrawPoints; i++) {
      fDrawPointsY[i] = axisMinCoord + ((fDrawPointsY[i]-a)/b)*(axisMaxCoord-axisMinCoord);
  }
   for (int i = 0; i < fNHistoPoints; i++) {
      fHistoPointsY[i] = axisMinCoord + ((fHistoPointsY[i]-a)/b)*(axisMaxCoord-axisMinCoord);
   }
   
}
