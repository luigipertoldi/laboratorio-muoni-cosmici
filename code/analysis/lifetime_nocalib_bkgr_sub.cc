/* Macro per la prima rozza analisi dati (1° semestre)
 * Mattia Faggin, Davide Piras, Luigi Pertoldi
 *
 * Usage: .x lifetime_nocalib_bkgr_sub( [inputFile] , [rebinFactor] , [midValue] )
 *
 * inputFile format: elenco dei file .txt con i dati, attenzione a non 
 *                   aggiungere spazi oppure newline characters
 *
 * Valori ottimali: midValue = 1000
 *                  rebin = 12
 */

#include <iostream>
#include <math.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#include "TApplication.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"

using namespace std;

#define m               1          //5.144E-03
#define calib           5.144E-03
#define sigmacalib      6.015E-6   // dati del fit di calibrazione canale - tempo (in microsecondi!)
#define q               0-150      //7.734E-01-1.635     
#define fitMin          30          
#define fitMax          2138
#define Tauptrue        2.1969803
#define sTauptrue       0.0000021
#define Taumtrue        0.88
#define sTaumtrue       0.01
#define RatioT          1.261
#define sRatioT         0.009
#define sigmaxstar      0          //incertezza sullo 0
                                                
int main ( int argc , char** argv ) {

    // salvo gli argomenti prima di darli in pasto al TApplication
    // perchè lui li modifica
    vector<string> args;
    args.reserve(argc);
    for ( int i = 0; i < argc; i++ ) args.push_back(argv[i]);

    if ( argc == 2 && args[1] == "--help" ) {
        std::cout << std::endl
                  << "AnalisiDati per la misura della vita media dei muoni cosmici in alluminio." << std::endl
                  << "Autori: Mattia Faggin, Davide Piras, Luigi Pertoldi" << std::endl << std::endl
                  << "Utilizzo:" << std::endl 
                  << "    $ ./lifetimeAnalysis [filelist] [rebinFactor] [midValue]" << std::endl << std::endl;
        return 0;
    }

    if ( argc < 4 ) {
        std::cout << "Pochi argomenti! Se non ti ricordi c'è l'opzione '--help'" << std::endl
                  << "Termino l'esecuzione..." << std::endl;
        return 0;
    }

    TApplication Root("App",&argc,argv);   
    
    ifstream file(args[1].c_str());
    vector<ifstream> files;
    string name;
    while ( file >> name ) {
        files.emplace_back(name.c_str());
    }

    int rebin = stoi(args[2]);
    int midValue = stoi(args[3]);
    
    // definiamo gli estremi dell'istogramma
    Double_t min = 0*m + q;
    Double_t max = 4096*m + q;
    TH1F data( "data" , "data" , 4096 , min , max);
        data.SetTitle("Spettro globale");
        data.SetYTitle("dN/dx");
        data.SetXTitle("Delay (canali)");
        data.SetLineColor(kBlack);
        data.SetStats(kFALSE);
        data.GetYaxis()->SetTitleOffset(1.5);
        data.SetLineColor(kBlue);

    TCanvas can( "ADCdataCan" , "ADC data" , 1500 , 600 );
    can.Divide(2);
    can.cd(1);
    gPad->SetGrid();

    string line;
    int totevents = 0;
    int height = 0;

    // data retrieving
    for ( int j = 0 ; j < files.size() ; j++ ) {
        for ( int i = 0 ; i < 8 ; i++ ) 
        { 
            std::getline(files[j],line);
        } // skip first lines
    }
    
    // initialize histogram
    for ( int i = 0 ; i < 4096 ; i++ ) data.SetBinContent( i, 0 );
    
    // fill histograms
    for ( int j = 0 ; j < files.size() ; j++ ) {
        for ( int i = 0 ; i < 4096 ; i++ ) {
    
            files[j] >> height;
            data.SetBinContent( i , data.GetBinContent(i)+height );
            totevents += height; // tot events
        }
        files[j].close();   // close the file
    }

    // dump number of events in title
    ostringstream convert;
    convert << totevents;
    string eventsstring = "Spettro Globale (" + convert.str() + " eventi)";
    data.SetTitle(eventsstring.c_str());
    
    /* // eventually dump as TText object
    TText * text = new TText( 0.5 , 0.94 , (eventsstring + " events").c_str());
    text->SetTextSize(0.04);
    text->SetTextAlign(22);
    text->SetNDC();
    text->Draw();
    */

    // REBIN
    data.Rebin(rebin);
    //data->Scale(1./rebinFactor);

    
    // compute mean background after rebinning
    int binThr = data.GetSize()-2;
    int k = 0;
    float center = 3001;
    while ( center > 3000 ) {
        center = data.GetBinCenter(binThr);
        binThr--;
        k++;
    }

    double mean = 0;
    int j;
    for( j = binThr; j < (data.GetSize()-2) ; j++ ) {
        mean += data.GetBinContent(j); 
        //cout << "data->GetBinContent(j) = " << data->GetBinContent(j) << endl;
        //cout << "mean = " << mean << endl;
    }

    // dump some infos
    cout << "\ndata.GetBinCenter("<<binThr<<") = " << data.GetBinCenter(binThr) << endl;
    cout << "Passaggi = "<< k << endl;
    cout << "binThr = " << binThr << endl;
    cout << "Canali = " << data.GetSize()-2 << endl;

    cout << "Sum = " << mean << endl;
    mean /= k;
    cout << "Bkgr = " << mean << endl;
    
    // background subtraction
    for ( int i = 0 ; i < data.GetSize() - 2 ; i++ ) data.SetBinContent( i , data.GetBinContent(i) - mean );

// ==================== COMMENTARE MEGLIO PLS ====================================

    // fit
    //TF1 * fminu = new TF1( "fminu" , "[0]+[1]*x", 0 , 20);
    //TF1 * fplus = new TF1( "fplus" , "[0]*exp(-x/[1])+[2]", q , 4096+q);
    TF1 fplus( "fplus" , "[0]*exp(-x/[1])", q , 4096+q);
    //fplus->FixParameter(2,mean);
    fplus.SetParameter(0,300);
    fplus.SetParameter(1,417);
    //fminu->SetLineColor(kBlue);

    //TFitResultPtr resultminu = dataLog->Fit( "fminu" , "SQR" , "" , fitMin , midValue );
    TFitResultPtr resultplus = data.Fit( "fplus" , "SQR" , "" , midValue , fitMax );
    
    //double tauM = -1./resultminu->Value(1);
    double tauP = resultplus->Value(1);
    double Aplus   = resultplus->Value(0);
    double sigmaAplus = sqrt(pow(resultplus->Error(0),2)+ pow(Aplus*sigmaxstar/tauP,2)) / rebin;
    double Nplus = Aplus * tauP / rebin; 
    double sigmaNplus = sqrt(pow(tauP, 2)*pow(resultplus->Error(0), 2) + pow(Aplus, 2)*pow(resultplus->Error(1), 2)+pow(Aplus,2)*pow(sigmaxstar,2) +2 * tauP*Aplus*resultplus->CovMatrix(0, 1))/rebin; 
    double sigmataupcalib = sqrt(pow(calib, 2)*pow(resultplus->Error(1), 2) + pow(resultplus->Value(1), 2)*pow(sigmacalib, 2));


    cout << endl;
    cout << "Primo fit a spanne (DX):\n";
    //cout << "tau_- = " << tauM << " +- " << resultminu->Error(1)/(tauM*tauM) << endl;
    cout << "tau+ = " << tauP << " +- " << resultplus->Error(1) << endl;
    cout << "tau+ calibrato = " << tauP*calib << " +- " << sigmataupcalib << endl;
    cout << "A+ = " << Aplus / rebin << " +- " << sigmaAplus << endl;
    //cout << "N+ = " << Nplus << " +- " << sigmaNplus << endl;
    cout << "Covariance check = " << resultplus->CovMatrix(0, 1) << endl<< endl;

    // draw
    //TCanvas * can = new TCanvas( "ADCdataCan" , "ADC data" , 1500 , 600 );
    //TCanvas * canLog = new TCanvas( "ADCdataCan (log)" , "ADC data (log)" , 1000 , 500 , 2050 , 725 ); // AppleTV
    
//  can->Divide(2);
    //can->cd();
    //gPad->SetGrid();
    //data->Draw("");
    //fplus->DrawCopy("SAME");  
    //canLog->SaveAs("datalog.png");
    
    // mu+ subtraction:
    // second fit to find A, t0
    //TF1 * f = new TF1( "f" , "log([0])+[1]/[2]-x/[2]" , 0 , 20); 
    //f->FixParameter( 2 , tauP );
    //f->SetParNames( "A" , "t0" );

    //TFitResultPtr result = dataLog->Fit ( "f" , "SQN" , "" , midValue , fitMax );
    
    //cout << "Secondo fit della seconda parte dello spettro (tau_-) per la stima dell'esponenziale da sottrarre:\n";
    //cout << "A = " << A << " +- " << result->Error(0) << endl;
    //cout << "t0 = " << t0 << " +- " << result->Error(1) << endl << endl;

    can.cd(2);
    gPad->SetGrid();

    TH1F dataSub( "dataSub" , "DataSub" , data.GetSize()-2 , min , max );
        dataSub.SetTitle("Spettro dei #mu^{-}");
        dataSub.SetXTitle("Delay (canali)");
        dataSub.SetYTitle("dN/dt");
        dataSub.SetStats(kFALSE);
        dataSub.GetYaxis()->SetTitleOffset(1.5);

    // subtraction
    for ( int i = 0 ; i < dataSub.GetSize()-2 ; i++ ) 
    {
        double content = data.GetBinContent(i) - fplus.Eval(data.GetBinCenter(i));
        dataSub.SetBinContent( i , content );
    }

    // rebinning & rescaling
    //dataSub->Rebin(rebin2);
    //dataSub->Scale(1./rebin2);
    
//  can->cd(2);

    // fitting the mu-
    //TF1 * fminu2 = new TF1( "fminu2" , "[0]*exp(-x/[1])+[2]*exp(-x/[3])+[4]", q , 4096+q);
    TF1 fminu2( "fminu2" , "[0]*exp(-x/[1])", q , 4096+q);
    fminu2.SetParameter(0,140);
    fminu2.SetParameter(1,194);
    TFitResultPtr resultminu2 = dataSub.Fit( "fminu2" , "SQR+" , "" , fitMin , midValue );

    double tauM2 = resultminu2->Value(1);
    double Aminus = resultminu2->Value(0);
    double sigmaAminus = sqrt(pow(resultminu2->Error(0), 2) + pow(Aminus*sigmaxstar / tauM2, 2)) / rebin;
    double Nminus = Aminus * tauM2/ rebin;
    double sigmaNminus = sqrt(pow(tauM2, 2)*pow(resultminu2->Error(0), 2) + pow(Aminus, 2)*pow(resultminu2->Error(1), 2) + pow(Aminus, 2)*pow(sigmaxstar, 2) + 2 * tauM2*Aminus*resultminu2->CovMatrix(0, 1))/rebin; //qui non si è tenuto conto dell'xstar
    double sigmataumcalib = sqrt(pow(calib, 2)*pow(resultminu2->Error(1), 2) + pow(resultminu2->Value(1), 2)*pow(sigmacalib, 2));

    cout << "Stima finale di tau- (SX):\n";
    cout << "tau- = " << tauM2 << " +- " << resultminu2->Error(1) << endl;
    cout << "tau- calibrato = " << tauM2*calib << " +- " << sigmataumcalib << endl;
    cout << "A- = " << Aminus / rebin << " +- " <<sigmaAminus << endl; 
    //cout << "N- = " << Nminus << " +- " << sigmaNminus << endl;
    cout << "Covariance check = " << resultminu2->CovMatrix(0, 1) << endl<< endl;
    
    double ratio = Nplus / Nminus;
    double sigmaratio = sqrt(pow(sigmaNplus,2) / pow(Nminus,2) + pow(Nplus,2)*pow(sigmaNminus,2) / pow(Nminus,4));
    double ratioA = Aplus / Aminus;
    double sigmaratioA = sqrt(pow(sigmaAplus, 2) / pow(Aminus/rebin, 2) + pow(Aplus/rebin, 2)*pow(sigmaAminus, 2) / pow(Aminus/rebin, 4));
    //cout << "Population ratio = " << ratio << " +- " << sigmaratio << endl;
    cout << "Ratio of A factors: " << ratioA << " +- " << sigmaratioA << endl;

    cout << endl;
    cout << "Compatibilita'" << endl;
    cout << "tau+: " << -(tauP*calib - Tauptrue) / sqrt(pow(sTauptrue, 2)+pow(sigmataupcalib, 2)) << endl;
    cout << "tau-: " << (tauM2*calib - Taumtrue) / sqrt(pow(sTaumtrue, 2)+pow(sigmataumcalib, 2)) << endl;
    cout << "Ratio: " << (ratioA - RatioT) / sqrt(pow(sigmaratioA, 2) + pow(sRatioT, 2)) << endl;

    double deltap, deltam, deltaA;
    deltap = tauP*calib-Tauptrue;
    deltam = tauM2*calib-Taumtrue;
    deltaA = ratioA - RatioT;
    cout<< "Delta+ = " << deltap << endl;
    cout<< "Delta- = " << deltam << endl;
    cout<< "DeltaA = " << deltaA << endl;

    //cout << "Population ratio: " << (ratio - RatioT) / sqrt(pow(sigmaratio, 2) + pow(sRatioT, 2)) << endl;

    
    // draw
    //TCanvas * can = new TCanvas( "canNew" , "canNew" , 1 ) ;
    //data->DrawCopy();

    //cout << "Il numero totale di eventi escluso il fondo e': " << data->Integral(-q/rebin, 4096/rebin) - mean*(4096+2*q) << endl; // - mean*(4096+q) 

    Root.Run();
    return 0;
}
