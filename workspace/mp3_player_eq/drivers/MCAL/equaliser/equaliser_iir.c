/***************************************************************************//**
  @file     equaliser.c
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "equaliser_iir.h"
#include "arm_math.h"
#include "math_helper.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define BLOCK_SIZE          (32)
#define IIR_EQ_GAIN_LEVELS  (8)     // Levels of gain
#define IIR_EQ_BANDS        (1)     // Equaliser bands
#define IIR_EQ_STAGES       (3)     // Stages per filter
#define IIR_EQ_COEFFS       (6)     // Coefficients per stages
#define IIR_EQ_STATE_VARS   (4)     // State var
#define IIR_EQ_FRAME_SIZE   (4096)  

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef struct
{
  uint8_t           gain;
}eq_iir_filter_t;

typedef struct
{        
  eq_iir_filter_t               filterBands[IIR_EQ_BANDS];                                      // Array that contains a filter-type for each band.
  arm_biquad_casd_df1_inst_q15  filter;                                                         // Actual filter instance used by ARM.
  float32_t                     coefficients[IIR_EQ_BANDS * IIR_EQ_COEFFS * IIR_EQ_STAGES];     // Must be converted to q15_t* before using it to initalise filter.
  q15_t                         coeffsInQ15[IIR_EQ_BANDS * IIR_EQ_STAGES * IIR_EQ_COEFFS];
  q15_t                         stateVars[IIR_EQ_STATE_VARS * IIR_EQ_STAGES * IIR_EQ_BANDS];    // State variables used by ARM for filtering with DSP module.
}eq_iir_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

void initBandWithGain(uint8_t);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
static const float32_t  equaliserCoeff[IIR_EQ_BANDS][IIR_EQ_GAIN_LEVELS][IIR_EQ_STAGES*IIR_EQ_COEFFS] = 
{
  {
    { 
      3.000457569435097*0.1,	0,	5.9315330317377*0.1,	2.9326543273445242*0.1,	0.9815512781295,	0.4822264076713645,	
      1.0,	0,	-0.78964784415202,	0.417080503749342,	-0.1032361501230595,	0.1745907762829685,	
      1.0,	0,	0.676795092148645,	-0.293273865969429,	0.44903583250402,	-0.03110570447655305,	
    },
    { 
      3.572487160079438,	0,	7.066188071343622,	3.4954106959726094,	0.98068792970962,	0.481429150918083,	
      1.0,	0,	-0.841409809007275,	0.4279632373639505,	-0.0719041504510775,	0.1737720765636555,	
      1.0,	0,	0.65595185225917,	-0.315038891334778,	0.4604910025392005,	-0.01914393328828345,	
    },
    { 
      4.252153428686023,	0,	8.41487845651657,	4.1645758279742555,	0.97978947097997,	0.4806030816547365,	
      1.0,	0,	-0.89169558156258,	0.439360017662088,	-0.04042505326278395,	0.1734880716843405,	
      1.0,	0,	0.63542118567379,	-0.3364775199437615,	0.471967227457113,	-0.00716017607197035,	
    },
    { 
      5.0591900094202895,	0,	10.016917702255737,	4.959730230169235,	-0.978855318055655,	-0.4797481814005205,	
      1.0,	0,	-0.940465778916815,	0.4512099086731465,	0.0088682415700173,	-0.1737454946241105,	
      1.0,	0,	0.615219189073485,	-0.357572943244936,	-0.4834513372188655,	-0.004831814692826065,	
    },
    { 
      6.016817918542399,	0,	11.918576612879834,	5.903924338392673,	0.97788502197374,	0.4788646101358925,	
      1.0,	0,	-0.98769048524474,	0.4634526362433925,	0.02269634913875295,	0.1745475904910315,	
      1.0,	0,	0.595360704537975,	-0.3783096627724535,	0.494930125553126,	0.016818248691738,	
    },
    { 
      7.390858546216912,	0,	14.64695846061342,	7.2585187668777325,	0.976878295656625,	0.4779527350389465,	
      0.967727400650245,	0,	-1.0,	0.460666400115422,	0.054198903450655,	0.175894068666925,	
      1.0,	0,	0.575859297524235,	-0.398673513141437,	0.50639041064568,	0.02878536114912335,	
    },
    { 
      9.15578332020058,	0,	18.15240323969603,	8.999344376577495,	0.97583504388803,	0.477013161916743,	
      0.928135728587675,	0,	-1.0,	0.4537486723566355,	0.085570389466976,	0.177781105530026,	
      1.0,	0,	0.55672724397697,	-0.4186516755075355,	0.51781909559046,	0.04071947595251795,	
    },
    { 
      11.301018877533316,	0,	22.41474004735133,	11.116778550698072,	0.974755396325975,	0.476046769238159,	
      0.89291648267105,	0,	-1.0,	0.4482041618744455,	0.116743198199405,	0.1802013962089555,	
      1.0,	0,	0.53797552624541,	-0.4382326818307105,	0.529203228014175,	0.0526070679141625,	
    },
  },
  // {
  //   { 
  //     0.7866787332791154,	0,	1.4605080271931106,	0.6874897567616478,	0.88284197936085,	0.404333988730602,	
  //     0.55322444362479,	0,	-1.0,	0.46316551506503,	-0.842171950627205,	0.3783009299045905,	
  //     1.0,	0,	0.0379670818538281,	-0.725805339466785,	0.028282793756512,	-0.29574434867409,	
  //   },
  //   { 
  //     1.0139914157224799,	0,	1.889522970217357,	0.891590328229241,	0.877011024416745,	0.4004992359427915,	
  //     0.550381004829755,	0,	-1.0,	0.464507040798399,	-0.834253491858985,	0.373663149172404,	
  //     1.0,	0,	0.0364226590025889,	-0.7369590146701,	0.02937773626256485,	-0.2878367778498055,	
  //   },
  //   { 
  //     1.3060404704244815,	0,	2.442322970862482,	1.1551431044738383,	0.87090968872961,	0.3965688620538015,	
  //     0.54770225627905,	0,	-1.0,	0.4658217011419205,	-0.82596723488863,	0.3689338377907925,	
  //     1.0,	0,	0.03493187662232535,	-0.74772530348468,	0.03050333339267335,	-0.2797078222409575,	
  //   },
  //   { 
  //     1.6808876202253917,	0,	3.1538079602913953,	1.4950499764512617,	0.86452924113901,	0.3925476946240855,	
  //     0.545177671661845,	0,	-1.0,	0.46710734284879,	-0.817301712040405,	0.364121403912938,	
  //     1.0,	0,	0.033493614783016,	-0.758112293938825,	0.0316595412978713,	-0.2713577985206175,	
  //   },
  //   { 
  //     2.1614956325967274,	0,	4.06842864250949,	1.9328716431105537,	0.857861286951195,	0.3884416592458995,	
  //     0.542797466268755,	0,	-1.0,	0.468362242582919,	-0.808245981398385,	0.359235674966181,	
  //     1.0,	0,	0.03210671449121115,	-0.76812835617318,	0.0328462414005891,	-0.262787563044734,	
  //   },
  //   { 
  //     2.777009978060934,	0,	5.242701539409631,	2.4960620807798164,	0.850897880495165,	0.3842578830750845,	
  //     0.540552540606765,	0,	-1.0,	0.4695850543208845,	-0.79878977998866,	0.35428799970725,	
  //     1.0,	0,	0.0307699840954652,	-0.777782096181355,	0.0340632368161494,	-0.2539985376957555,	
  //   },
  //   { 
  //     3.56435969197834,	0,	6.748354056636693,	3.2195139154534913,	0.84363165056569,	0.3800047993259385,	
  //     0.53843442849589,	0,	-1.0,	0.4707747625075105,	-0.788923692305435,	0.3492913454771185,	
  //     1.0,	0,	0.02948220533658815,	-0.78708231211556,	0.0353102490354629,	-0.244992733839883,	
  //   },
  //   { 
  //     4.570251953412632,	0,	8.6762473587341,	4.147487970757088,	0.83605593889138,	0.375692250568629,	
  //     0.53643524928863,	0,	-1.0,	0.4719306403536935,	-0.77863933392377,	0.344260387461417,	
  //     1.0,	0,	0.02824213903547195,	-0.79603795320966,	0.0365869149174924,	-0.23577277404721,	
  //   },
  // },
  // {
  //   { 
  //     0.6085341569361123,	0,	-1.1837109894973876,	0.5769116006101498,	-0.969867312779925,	0.4715862097410075,	
  //     0.808953699216515,	0,	1.0,	0.4399268141185955,	0.579907738324685,	0.25908264963028,	
  //     1.0,	0,	-0.451037990935175,	-0.4690889127875635,	-0.24176510464558,	-0.2154213677404815,	
  //   },
  //   { 
  //     0.7798206462811931,	0,	-1.5188796682527315,	0.7410829421563564,	-0.968410795556645,	0.4702979406918965,	
  //     0.78608675028452,	0,	1.0,	0.4376922223542955,	0.559700390727385,	0.252793574273024,	
  //     1.0,	0,	-0.435324804709461,	-0.487584704606272,	-0.25014728749019,	-0.2055548068372225,	
  //   },
  //   { 
  //     0.9972209653460118,	0,	-1.9447415706509492,	0.9498765997416545,	-0.966888586681035,	0.4689600747812755,	
  //     0.765326035698345,	0,	1.0,	0.4360922229821395,	0.53874309089663,	0.2466242869826435,	
  //     1.0,	0,	-0.4199748277636745,	-0.505652967397345,	-0.2586963904819985,	-0.195491766349965,	
  //   },
  //   { 
  //     1.2726165593503618,	0,	-2.4847627820624414,	1.214882819175975,	-0.96529853470788,	0.4675719303464205,	
  //     0.746427402676105,	0,	1.0,	0.435032589817232,	0.51703765718306,	0.2406037854885255,	
  //     1.0,	0,	-0.4049926552281675,	-0.523288292291235,	-0.2674073243441065,	-0.1852382368018695,	
  //   },
  //   { 
  //     1.6208027038187054,	0,	-3.168184295950854,	1.550553522105261,	-0.963638554091355,	0.466133012341605,	
  //     0.729181756811865,	0,	1.0,	0.43443306745296,	0.4945896571043005,	0.2347622514718375,	
  //     1.0,	0,	-0.39038183919213,	-0.540486498168905,	-0.2762744262472675,	-0.174800883837278,	
  //   },
  //   { 
  //     2.0601618656105942,	0,	-4.031360394694743,	1.9748673662962848,	-0.96190665261256,	0.4646430469383545,	
  //     0.71340901157684,	0,	1.0,	0.434224997175622,	0.4714086669900695,	0.229130861975613,	
  //     1.0,	0,	-0.376144925063921,	-0.55724458886252,	-0.2852914630851525,	-0.164187044366438,	
  //   },
  //   { 
  //     2.613481604585927,	0,	-5.119385006959943,	2.51013824082451,	-0.96010096294947,	0.4631020200812265,	
  //     0.69895325213261,	0,	1.0,	0.4343494149281535,	0.4475085026336555,	0.223741567793767,	
  //     1.0,	0,	-0.362283491811377,	-0.573560705789105,	-0.2944516387154105,	-0.153404718042144,	
  //   },
  //   { 
  //     3.308945310530403,	0,	-6.488062523059579,	3.1839953823382237,	-0.958219778754295,	0.461510220184142,	
  //     0.68567883793378,	0,	1.0,	0.434755514309816,	0.4229074136777575,	0.218626839209805,	
  //     1.0,	0,	-0.3487981954851085,	-0.589434076719825,	-0.303747605295147,	-0.142462553916888,	
  //   },
  // },
  // {
  //   { 
  //     0.4686781593668006,	0,	-0.9297751444365631,	0.46121992053664507,	-0.99112405585555,	0.4912825117906875,	
  //     1.0,	0,	-0.136122677299149,	0.3416349248295545,	-0.131038510741335,	0.174541280376109,	
  //     0.96025017567911,	0,	-1.0,	0.054545523950279,	-0.54417495443732,	0.052226403609009,	
  //   },
  //   { 
  //     0.5215383705348379,	0,	-1.0350255583852395,	0.5136116252355182,	-0.99070401822934,	0.4908781579161875,	
  //     1.0,	0,	-0.072615264305328,	0.3394524961854765,	-0.1621263627905555,	0.1771534991186835,	
  //     0.982503216310835,	0,	-1.0,	0.03229248331855395,	-0.55583235527356,	0.0640562838464835,	
  //   },
  //   { 
  //     0.5834522922996115,	0,	-1.1583098215154706,	0.5749841458567422,	-0.99026598814058,	0.4904573378441415,	
  //     1.0,	0,	-0.0089415928122729,	0.338313885265886,	-0.19286111150432,	0.180247193601497,	
  //     1.0,	0,	-0.99418425540942,	0.0088939070287267,	-0.5674183471937,	0.0758136986229825,	
  //   },
  //   { 
  //     0.6643946729995115,	0,	-1.3194512411122943,	0.6551876958604546,	-0.98980949499174,	0.4900197241441675,	
  //     1.0,	0,	0.0547551250769335,	0.3382227108371825,	-0.223182483411891,	0.183804788140207,	
  //     1.0,	0,	-0.970550058542795,	-0.0150899743157199,	-0.57892038812874,	0.087485920300821,	
  //   },
  //   { 
  //     0.7566482410973805,	0,	-1.5031507242879476,	0.746638286667775,	-0.98933410632086,	0.4895650398193615,	
  //     1.0,	0,	0.118331143852883,	0.33917563337684,	-0.253034131599213,	0.18780619762417,	
  //     1.0,	0,	-0.946932960388315,	-0.03905650396061455,	-0.590326308615375,	0.099060599360976,	
  //   },
  //   { 
  //     0.8618268211189171,	0,	-1.712629961834447,	0.8509437951894652,	-0.9888394380367,	0.4890930693495355,	
  //     1.0,	0,	0.181643617286025,	0.3411623849945245,	-0.2823640264204055,	0.1922291202310775,	
  //     1.0,	0,	-0.92336006042649,	-0.062978181469667,	-0.601624360764255,	0.1105258140958475,	
  //   },
  //   { 
  //     0.9817809132922181,	0,	-1.9515834031864037,	0.9699481819021023,	-0.988325166108485,	0.488603671183327,	
  //     1.0,	0,	0.2445520126895005,	0.344165909506964,	-0.3111247633134915,	0.1970493487537835,	
  //     1.0,	0,	-0.89985825543991,	-0.086827712103578,	-0.612803263371985,	0.121870116388747,	
  //   },
  //   { 
  //     1.118633186904488,	0,	-2.224249183792355,	1.105766924869374,	-0.987791039832715,	0.48809679178394,	
  //     1.0,	0,	0.306919476202192,	0.3481626089304675,	-0.3392737855433435,	0.2022410925218245,	
  //     1.0,	0,	-0.876454117355845,	-0.110578130784819,	-0.62385224284501,	0.133082573243264,	
  //   },
  // },
  // {
  //   { 
  //     0.05679817852842434,	0,	-0.1130784432061292,	0.05628504960394426,	-0.99499835286718,	0.495049261910547,	
  //     1.0,	0,	-0.83721599223533,	0.42885651530255,	-0.4686503560919775,	0.2267764234617145,	
  //     0.757264921480215,	0,	-1.0,	0.247446718393611,	-0.681249754254465,	0.1844595577606455,	
  //   },
  //   { 
  //     0.0608024764678673,	0,	-0.12107573931753315,	0.06027792144370364,	-0.994762726603815,	0.4948186847182765,	
  //     1.0,	0,	-0.78491901805867,	0.4172237628506025,	-0.4925121408716385,	0.233250555166483,	
  //     0.76979108568443,	0,	-1.0,	0.234920554189393,	-0.69146919886845,	0.194727152717362,	
  //   },
  //   { 
  //     0.0650669817664663,	0,	-0.12959346242263461,	0.06453101431346427,	-0.99451712725082,	0.494578627284524,	
  //     1.0,	0,	-0.731178208568445,	0.4061453315515175,	-0.51558741298209,	0.2398870513766515,	
  //     0.78293278220476,	0,	-1.0,	0.221778857669063,	-0.70149952334737,	0.204804736473042,	
  //   },
  //   { 
  //     0.06961267857863439,	0,	-0.1386735393965687,	0.06906527139960526,	-0.994261310284775,	0.494328891530181,	
  //     1.0,	0,	-0.67605077634463,	0.3956787642766175,	-0.53787145170338,	0.24665857927018,	
  //     0.796720258132495,	0,	-1.0,	0.2079913817413325,	-0.711334714690865,	0.2146862676962385,	
  //   },
  //   { 
  //     0.07446283976940953,	0,	-0.14836247171354108,	0.07390392172162198,	-0.99399505513309,	0.494069307345742,	
  //     1.0,	0,	-0.61960433370252,	0.3858804585440135,	-0.55936354911437,	0.253538770573575,	
  //     0.811185246892785,	0,	-1.0,	0.1935263929810395,	-0.720969431438045,	0.224366379759019,	
  //   },
  //   { 
  //     0.07964325557467349,	0,	-0.158711793053795,	0.07907270908019952,	-0.9937181712313,	0.4937997389165765,	
  //     1.0,	0,	-0.56191687051144,	0.3768050042447705,	-0.58006668193704,	0.2605023771312565,	
  //     0.82636104128272,	0,	-1.0,	0.178350598591106,	-0.730398999154005,	0.23384037620222,	
  //   },
  //   { 
  //     0.08518248151022773,	0,	-0.16977856539307182,	0.08460014023951892,	-0.993430504931065,	0.4935200918976,	
  //     1.0,	0,	-0.503076575111065,	0.3685045046736805,	-0.599987168524005,	0.267525395765235,	
  //     0.8422825700984,	0,	-1.0,	0.162429069775423,	-0.73961940195851,	0.2431042222242315,	
  //   },
  //   { 
  //     0.09111210770848492,	0,	-0.181625917927479,	0.09051775451784592,	-0.993131947329655,	0.4932303215024365,	
  //     1.0,	0,	-0.443181490236133,	0.3610278945309355,	-0.619134318256205,	0.274585163349822,	
  //     0.858986478527525,	0,	-1.0,	0.1457251613463015,	-0.748627270396705,	0.2521545324945385,	
  //   },
  // },
  // {
  //   { 
  //     0.09515606568465088,	0,	-0.1898923221797774,	0.0947381466008797,	-0.997580682484855,	0.497592688597326,	
  //     0.695673169932465,	0,	-1.0,	0.435194024765502,	-0.745942168346635,	0.325773459771386,	
  //     0.62396270261333,	0,	-1.0,	0.3771436170692735,	-0.81609616038614,	0.3169990236312725,	
  //   },
  //   { 
  //     0.09648473231780076,	0,	-0.1925631032865503,	0.09608011379752827,	-0.997467066426355,	0.497480264881198,	
  //     0.710082563774045,	0,	-1.0,	0.434785211291485,	-0.758616288033905,	0.332105488219408,	
  //     0.63002701411798,	0,	-1.0,	0.37107930556462,	-0.82315174058331,	0.3240624095556835,	
  //   },
  //   { 
  //     0.09767283405631642,	0,	-0.19495302072772544,	0.09728179095098,	-0.99734868221618,	0.497363189634526,	
  //     0.725818393075385,	0,	-1.0,	0.4346743744555805,	-0.77069912042735,	0.3383185986659835,	
  //     0.63638932424737,	0,	-1.0,	0.36471699543523,	-0.82999135523119,	0.3309095910038695,	
  //   },
  //   { 
  //     0.09871034575671432,	0,	-0.19704197927587747,	0.09833310768399539,	-0.997225418600405,	0.497241362626697,	
  //     0.743041695324405,	0,	-1.0,	0.43491508663507,	-0.782213730449695,	0.3444059086776995,	
  //     0.6430642765732,	0,	-1.0,	0.3580420431094005,	-0.83661726046159,	0.3375428266036405,	
  //   },
  //   { 
  //     0.09958651096504448,	0,	-0.19880842512802102,	0.09922326633201763,	-0.99709717675386,	0.497114697014244,	
  //     0.76193916885843,	0,	-1.0,	0.4355698274213105,	-0.793183098595075,	0.3503616605156195,	
  //     0.65006723424832,	0,	-1.0,	0.351039085434282,	-0.843032049867005,	0.343964712816839,	
  //   },
  //   { 
  //     0.10028972695758408,	0,	-0.20022911594770393,	0.09994062695047448,	-0.99696387330979,	0.4969831224349235,	
  //     0.782728456698595,	0,	-1.0,	0.436711765636395,	-0.803629989718095,	0.35618114131428,	
  //     0.657414315366745,	0,	-1.0,	0.343692004315855,	-0.849238624582155,	0.3501781539877565,	
  //   },
  //   { 
  //     0.10080741985809481,	0,	-0.2012788710286784,	0.10047258236641234,	-0.996825443811705,	0.496846588523874,	
  //     0.80566479667678,	0,	-1.0,	0.438426996099898,	-0.813576839690975,	0.361860602327972,	
  //     0.665122430061275,	0,	-1.0,	0.335983889621326,	-0.85524016391744,	0.3561863329441305,	
  //   },
  //   { 
  //     0.10112590901372391,	0,	-0.20193029995019757,	0.10080542246246177,	-0.99668184662235,	0.496705068885315,	
  //     0.83104946923208,	0,	-1.0,	0.440817374881892,	-0.82304565856769,	0.3673971782355535,	
  //     0.673209319424045,	0,	-1.0,	0.327897000258556,	-0.86104009671247,	0.361992682318971,	
  //   },
  // },
  // {
  //   { 
  //     0.012745367817192071,	0,	-0.0254652123842799,	0.012719896921339966,	-0.998902000443275,	0.4989044835927265,	
  //     0.56911805336402,	0,	-1.0,	0.456578595419289,	-0.89044632458278,	0.409328192322915,	
  //     0.556176647069735,	0,	-1.0,	0.4440517643098015,	-0.907362105443685,	0.4075693572739305,	
  //   },
  //   { 
  //     0.012849874363234889,	0,	-0.025675181969561853,	0.012825355604122502,	-0.99885052105304,	0.498853250976133,	
  //     0.573306618610995,	0,	-1.0,	0.4549897264867555,	-0.895953043638755,	0.4132168403633415,	
  //     0.558931540253865,	0,	-1.0,	0.4412968711256695,	-0.91130740786016,	0.411515560842372,	
  //   },
  //   { 
  //     0.012947857212610383,	0,	-0.02587208530691933,	0.01292427206950336,	-0.99879689097957,	0.4987998918376675,	
  //     0.57778192428309,	0,	-1.0,	0.4533836873178945,	-0.901186051670875,	0.4169683196253535,	
  //     0.561821808142865,	0,	-1.0,	0.43840660323667,	-0.915099988970145,	0.4153090082210415,	
  //   },
  //   { 
  //     0.013039238634281965,	0,	-0.02605576587566623,	0.013016567505438796,	-0.9987410614141,	0.4987443596558495,	
  //     0.58256785587017,	0,	-1.0,	0.451765405162709,	-0.906159359782085,	0.420585673130398,	
  //     0.564854103013065,	0,	-1.0,	0.4353743083664735,	-0.91874443366856,	0.4189542853520955,	
  //   },
  //   { 
  //     0.013123922734322412,	0,	-0.026226031043748536,	0.01310214515245031,	-0.9986829893915,	0.498686613952129,	
  //     0.58769071606919,	0,	-1.0,	0.450140659649716,	-0.9108863239048,	0.4240720967533375,	
  //     0.56803540403184,	0,	-1.0,	0.4321930073476945,	-0.922245303258365,	0.4224559545803545,	
  //   },
  //   { 
  //     0.013201792756960857,	0,	-0.026382646649472624,	0.013180887584332054,	-0.99862263918968,	0.4986266217037965,	
  //     0.593179523208275,	0,	-1.0,	0.4485161960641945,	-0.91537966176103,	0.4274309071227765,	
  //     0.571373033320985,	0,	-1.0,	0.4288553780585515,	-0.925607124118295,	0.4258185433184225,	
  //   },
  //   { 
  //     0.01327270824261119,	0,	-0.026525331298490513,	0.013252653847115374,	-0.99855998392377,	0.4985643589517895,	
  //     0.59906635446445,	0,	-1.0,	0.4468998543838165,	-0.9196514711217,	0.430665512259637,	
  //     0.574874672809355,	0,	-1.0,	0.4253537385701825,	-0.928834377488655,	0.4290465338301785,	
  //   },
  //   { 
  //     0.01333650201840494,	0,	-0.026653750327088083,	0.01331727643182262,	-0.998495007347395,	0.4984998126146495,	
  //     0.60538674192041,	0,	-1.0,	0.44530071674596,	-0.923713248952525,	0.433779384760803,	
  //     0.578548381913715,	0,	-1.0,	0.421680029465822,	-0.93193149032411,	0.4321443540814465,	
  //   },
  // },
  // {
  //   { 
  //     0.014098166065445904,	0,	-0.02818090623948909,	0.014082757487401011,	-0.999400093178535,	0.499400835583742,	
  //     0.534290490789215,	0,	-1.0,	0.473319839519091,	-0.942024723623915,	0.447949948575291,	
  //     0.53067832131346,	0,	-1.0,	0.4693899290497255,	-0.947194088757815,	0.4472587350983805,	
  //   },
  //   { 
  //     0.014166050820520757,	0,	-0.028317303332844025,	0.01415126833140947,	-0.999371984393815,	0.4993728005977735,	
  //     0.53620777273273,	0,	-1.0,	0.472164921850312,	-0.944899585409745,	0.450303827550868,	
  //     0.532184168810985,	0,	-1.0,	0.467884081552206,	-0.949539738220055,	0.4496045446520485,	
  //   },
  //   { 
  //     0.014230268454818078,	0,	-0.028446346767219077,	0.01421609276096022,	-0.99934270333321,	0.499343600564205,	
  //     0.538240044607565,	0,	-1.0,	0.4709724702088975,	-0.94763342669605,	0.452562429108325,	
  //     0.53376401324665,	0,	-1.0,	0.466304237116537,	-0.951786361609335,	0.4518513213741905,	
  //   },
  //   { 
  //     0.014290871543888662,	0,	-0.02856814169150482,	0.014277283339823412,	-0.99931222368167,	0.499313209852738,	
  //     0.540395365055395,	0,	-1.0,	0.4697423662685195,	-0.950233592025445,	0.4547291740221635,	
  //     0.535421490809965,	0,	-1.0,	0.464646759553222,	-0.953937675589445,	0.454002782182264,	

  //     // 0.500000,0.000000,-0.965247,0.469577,0.920261,-0.429433,0.500000,0.000000,-0.984162,0.485172,0.983325,-0.483781,0.081832,0.000000,0.163664,0.081832,0.316046,-0.183314
  //   },
  //   { 
  //     0.014347904066042208,	0,	-0.02868277614878686,	0.014334884123832005,	-0.99928052235982,	0.4992816061284505,	
  //     0.542682455129915,	0,	-1.0,	0.4684746595080365,	-0.95270703924072,	0.4568074212521005,	
  //     0.53716041637179,	0,	-1.0,	0.4629078339913965,	-0.95599729821308,	0.456062545375889,	
  //   },
  //   { 
  //     0.014401400681997568,	0,	-0.02879031962369338,	0.01438892992857641,	-0.999247580292105,	0.499248771123943,	
  //     0.545110762901735,	0,	-1.0,	0.467169592768622,	-0.95506035793597,	0.4588004611882455,	
  //     0.538984792264715,	0,	-1.0,	0.4610834580984745,	-0.957968747727125,	0.4580341294420815,	
  //   },
  //   { 
  //     0.014451385975394035,	0,	-0.028890821512435726,	0.014439445558895113,	-0.999213383283195,	0.499214691519679,	
  //     0.547690535526735,	0,	-1.0,	0.465827630856164,	-0.95729978708717,	0.4607115097933645,	
  //     0.540898817494855,	0,	-1.0,	0.4591694328683315,	-0.95985544183479,	0.459920952317303,	
  //   },
  //   { 
  //     0.014497873645524713,	0,	-0.028984309498480822,	0.014486444991777965,	-0.999177923011645,	0.4991793599419485,	
  //     0.55043289978363,	0,	-1.0,	0.4644494925572685,	-0.959431231833775,	0.4625437035306585,	
  //     0.54290689740637,	0,	-1.0,	0.4571613529568185,	-0.96166069737152,	0.4617263310633775,	
  //   },
  // },
};


/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static eq_iir_context_t context;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
void eqIirInit(void)
{
  for (uint16_t band = 0; band < IIR_EQ_BANDS; band++)
  {
    context.filterBands[band].gain = 3;
    initBandWithGain(band);
  }
  
  arm_float_to_q15(context.coefficients, context.coeffsInQ15, IIR_EQ_BANDS * IIR_EQ_STAGES * IIR_EQ_COEFFS);
  arm_biquad_cascade_df1_init_q15(&context.filter, IIR_EQ_STAGES, context.coeffsInQ15, context.stateVars, 1);
}

void eqIirFilterFrame(uint16_t * inputF32, uint16_t * outputF32)
{
  arm_biquad_cascade_df1_q15(&(context.filter), (q15_t*) inputF32, (q15_t*) outputF32, IIR_EQ_FRAME_SIZE);
}

void eqIirSetFilterGain(uint32_t band, uint32_t gain)
{
  // Gain must be between 0 and 7.
  context.filterBands[band].gain = gain;
  initBandWithGain(band);
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void initBandWithGain(uint8_t band)
{
  for (uint16_t j = 0; j < IIR_EQ_STAGES*IIR_EQ_COEFFS; j++)
  { 
    context.coefficients[band*IIR_EQ_STAGES*IIR_EQ_COEFFS + j] = equaliserCoeff[band][context.filterBands[band].gain][j];
  }
}

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
