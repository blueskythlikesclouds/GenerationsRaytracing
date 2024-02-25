#include "EnvironmentColor.h"

#include "Device.h"

static const xxHashMap<std::array<uint8_t, 6>> s_environmentColors =
{
    // Sonic Unleashed
    { 0x3ce52f71f53a0333, { 208, 250, 244 } }, // ActD_Africa
    { 0xa2106d5402d979d7, { 149, 150, 198 } }, // ActD_Beach
    { 0x817a829098cf2f6c, { 117, 163, 193 } }, // ActD_China
    { 0x334ddc94268b6d,   { 255, 255, 255 } }, // ActD_EU, ActD_SubEU_04
    { 0x4819bbf1ed1b4092, { 62, 76, 179 } },   // ActD_MykonosAct1
    { 0x12f6c7fdafa98428, { 99, 99, 198 } },   // ActD_MykonosAct2
    { 0xf4db322c7ea87c9a, { 213, 250, 255 } }, // ActD_NY
    { 0x1eb7f67273cecaa,  { 106, 138, 149 } }, // ActD_Petra
    { 0xf1bc7a919097a961, { 128, 167, 255 } }, // ActD_Snow
    { 0x15cac74bd472f4b1, { 209, 233, 249 } }, // ActD_SubAfrica_01
    { 0x645c135b015f5c51, { 209, 251, 243 } }, // ActD_SubAfrica_02
    { 0x4bc8c6b374ef7dc,  { 86, 91, 166 } },   // ActD_SubAfrica_03
    { 0xc41c004f7d1be395, { 144, 145, 193 } }, // ActD_SubBeach_01
    { 0x23bcb56a132034d,  { 112, 164, 249 } }, // ActD_SubBeach_02
    { 0x9242344c3fc33633, { 139, 142, 188 } }, // ActD_SubBeach_03, ActD_SubBeach_04
    { 0x90b9c2aeafc410d5, { 127, 181, 209 } }, // ActD_SubChina_01
    { 0x2659ddb41ff9093e, { 166, 183, 179 } }, // ActD_SubChina_02
    { 0xbfe9a6ad43d2585e, { 116, 161, 193 } }, // ActD_SubChina_03
    { 0xae20dce270738af3, { 105, 142, 166 } }, // ActD_SubChina_04
    { 0x5d60949144ba996b, { 97, 106, 139 } },  // ActD_SubEU_01
    { 0xeb5ebf08dff02c84, { 224, 235, 255 } }, // ActD_SubEU_02
    { 0x3100bff8a77d24ea, { 188, 163, 218 } }, // ActD_SubEU_03
    { 0xab90b70e315778b9, { 123, 73, 198 } },  // ActD_SubMykonos_01
    { 0xb716bf4ce541719e, { 59, 71, 173 } },   // ActD_SubMykonos_02
    { 0x305282b50ee18452, { 193, 232, 239 } }, // ActD_SubNY_01
    { 0xa5a5a505eeb492d,  { 209, 247, 255 } }, // ActD_SubNY_02
    { 0xd1c5ec1d1a4d1840, { 97, 138, 156 } },  // ActD_SubPetra_02
    { 0xa21577b8df3b33b9, { 105, 135, 148 } }, // ActD_SubPetra_03
    { 0xee3d8260fdd7ae2a, { 97, 152, 249 } },  // ActD_SubSnow_01
    { 0xa079eb43b350b845, { 116, 109, 193 } }, // ActD_SubSnow_02
    { 0x1f2e36dd741ee6de, { 135, 171, 249 } }, // ActD_SubSnow_03
    { 0xfd638a08c4fa6069, { 2, 64, 156 } },    // ActN_AfricaEvil
    { 0x2f8d2049ca5ae027, { 0, 48, 135 } },    // ActN_BeachEvil
    { 0x6e9ce45e4d59ac01, { 0, 129, 255 } },   // ActN_ChinaEvil
    { 0x1c76c4c61c6ecdc1, { 16, 39, 65 } },    // ActN_EUEvil
    { 0x7ad18ba0ee8d2eff, { 48, 36, 78 } },    // ActN_MykonosEvil
    { 0x2302996784b753,   { 2, 9, 12 } },      // ActN_NYEvil, ActN_SubNY_01
    { 0x6e34fcae86a64530, { 38, 82, 209 } },   // ActN_PetraEvil
    { 0xfac157022da22bd,  { 101, 119, 144 } }, // ActN_SnowEvil
    { 0x68605862784bc5c5, { 8, 51, 90 } },     // ActN_SubAfrica_01
    { 0x49b8c45da08b33f0, { 45, 79, 153 } },   // ActN_SubAfrica_02
    { 0x68a708dcd81f8658, { 33, 101, 198 } },  // ActN_SubAfrica_03
    { 0xf0baa13c344d5b7c, { 45, 52, 74 } },    // ActN_SubBeach_01, ActN_SubEU_01
    { 0xb837ced9b6c8d205, { 0, 126, 255 } },   // ActN_SubChina_01
    { 0x8be72a684ae495e1, { 0, 145, 255 } },   // ActN_SubChina_02
    { 0x1fa2f762e59e8fdc, { 31, 18, 65 } },    // ActN_SubMykonos_01, Town_Mykonos_Night
    { 0x75818bc493e2a5bd, { 80, 74, 116 } },   // ActN_SubPetra_02
    { 0x3b3a0de6eb98dce2, { 48, 63, 97 } },    // ActN_SubSnow_01
    { 0x64d0e9981311c067, { 38, 59, 108 } },   // ActN_SubSnow_02
    { 0xa46a4242dd733faf, { 59, 22, 22 } },    // Act_EggmanLand
    { 0x2fc1f50800c472e4, { 153, 58, 56 } },   // BossDarkGaia1_1Air, BossFinalDarkGaia
    { 0x9877b9309c1d1f31, { 40, 37, 93 } },    // BossDarkGaia1_1Run
    { 0x35d6d5696feebe6a, { 45, 43, 108 } },   // BossDarkGaia1_2Run
    { 0x1330c7b8c3bf1c49, { 56, 52, 139 } },   // BossDarkGaia1_3Run
    { 0xfbc04b53e8f43304, { 18, 40, 59 } },    // BossDarkGaiaMoray
    { 0x5d7089d8ec4e03c5, { 193, 232, 224 } }, // BossEggBeetle
    { 0x465d0b2de5eb7715, { 93, 82, 79 } },    // BossEggDragoon, Event_M8_03
    { 0xc0cf061f57d9ae1f, { 153, 152, 153 } }, // BossEggLancer
    { 0x7044d7e066764b10, { 213, 218, 239 } }, // BossEggRayBird
    { 0xf1b405db5fc8be20, { 6, 1, 31 } },      // BossPetra
    { 0xdf19e846d0229d5f, { 0, 25, 56 } },     // BossPhoenix
    { 0x3e8825ecb8a4bb0a, { 0, 0, 0 } },       // Event_M2_01_professor_room_new
    { 0x664180b711d2b5c3, { 0, 0, 0 } },       // Event_M4_01_egb_hideout
    { 0xd79de450061ca6f9, { 0, 0, 0 } },       // Event_M6_01_temple
    { 0x3cd2235592697129, { 0, 0, 0 } },       // Event_M8_02_egbHall
    { 0xf30d20a7465c9e33, { 0, 0, 0 } },       // Event_M8_16_myk
    { 0xc6289e5223a2d0ab, { 0, 0, 0 } },       // Event_afr_hideout
    { 0x6d174a20a194d4cb, { 0, 0, 0 } },       // Event_egb_hidout_exterior
    { 0x38ce5a5b25b24596, { 0, 0, 0 } },       // Event_temple
    { 0xeb5c0e38aab9b282, { 0, 0, 0 } },       // ExStageTails1
    { 0x304c2eb9550248dc, { 0, 0, 0 } },       // ExStageTails2
    { 0x7ca14cdc575ce805, { 229, 204, 178 } }, // Title
    { 0x15f8f352b5d4bf90, { 0, 0, 0 } },       // Town_Africa
    { 0x52666cacba8a4e74, { 0, 0, 0 } },       // Town_AfricaETF
    { 0x41adb4f67aac7fc2, { 0, 0, 0 } },       // Town_AfricaETF_Night, Town_Africa_Night
    { 0x44ce7f4a79ae3b93, { 0, 0, 0 } },       // Town_China
    { 0x5e1eaa2a3d663c51, { 0, 0, 0 } },       // Town_ChinaETF
    { 0x7c01942524da64a5, { 0, 0, 0 } },       // Town_ChinaETF_Night
    { 0xfc8af43b5cbfdb85, { 0, 0, 0 } },       // Town_China_Night
    { 0xd9c24d3dcb63dde6, { 0, 0, 0 } },       // Town_EULabo
    { 0xfbe967b2a8b8b1d6, { 0, 0, 0 } },       // Town_EULabo_Night
    { 0x7ad42362129125b2, { 0, 0, 0 } },       // Town_EggManBase
    { 0x3fa95645a74c7cf4, { 0, 0, 0 } },       // Town_EuropeanCity
    { 0xf75fc17b26330ae3, { 0, 0, 0 } },       // Town_EuropeanCityETF
    { 0x7de6f88c522e7c67, { 0, 0, 0 } },       // Town_EuropeanCityETF_Night
    { 0x65cd567905eb7844, { 0, 0, 0 } },       // Town_EuropeanCity_Night
    { 0x54d3f511756ff087, { 0, 0, 0 } },       // Town_Mykonos, Town_MykonosETF
    { 0x10f37d9e1beb61be, { 0, 0, 0 } },       // Town_MykonosETF_Night
    { 0x8d473a531b0265f3, { 0, 0, 0 } },       // Town_NYCity, Town_NYCityETF
    { 0xee63a04ed4a8cff1, { 0, 0, 0 } },       // Town_NYCityETF_Night, Town_NYCity_Night
    { 0xe6349f36439d049e, { 0, 0, 0 } },       // Town_PetraCapital
    { 0x371ef599306f6ce9, { 0, 0, 0 } },       // Town_PetraCapitalETF
    { 0xb0df4904feb5b2cd, { 0, 0, 0 } },       // Town_PetraCapitalETF_Night
    { 0xbc6f1afb37427571, { 0, 0, 0 } },       // Town_PetraCapital_Night
    { 0x25604274471fdf8e, { 0, 0, 0 } },       // Town_PetraLabo
    { 0x76cfff7968c3fc73, { 0, 0, 0 } },       // Town_PetraLabo_Night
    { 0x5f29d638614702ac, { 0, 0, 0 } },       // Town_Snow
    { 0xf6e80cb4e6a38672, { 0, 0, 0 } },       // Town_SnowETF
    { 0x717fbd7fc699e8e5, { 0, 0, 0 } },       // Town_SnowETF_Night
    { 0x9391f714e425497d, { 0, 0, 0 } },       // Town_Snow_Night
    { 0xbffebd9559e2621c, { 0, 0, 0 } },       // Town_SouthEastAsia
    { 0xc91c3fe744b30065, { 0, 0, 0 } },       // Town_SouthEastAsiaETF
    { 0xc479ad58cdf397aa, { 0, 0, 0 } },       // Town_SouthEastAsiaETF_Night, Town_SouthEastAsia_Night

    // Sonic Generations
    { 0xdea040012b6e6dee, { 56, 96, 116 } },                 // bde
    { 0xb267227486fec93,  { 71, 71, 71 } },                  // blb
    { 0xa0fd611c40e85bee, { 33, 1, 10 } },                   // bms
    { 0x7dac5b26699920d,  { 239, 134, 113 } },               // bne
    { 0xdfee19606fc49127, { 65, 101, 132 } },                // bpc
    { 0x7b6eb97f99818eb6, { 16, 30, 43 } },                  // bsd
    { 0x87f6cecb05549bfd, { 153, 93, 77 } },                 // bsl
    { 0xa72a03b46543d1b8, { 38, 27, 84 } },                  // cnz100
    { 0xac3c2e9c794443bb, { 62, 18, 15 } },                  // cpz100
    { 0xcdb2f1065b8e9656, { 48, 49, 107 } },                 // cpz200
    { 0xbb6a4511aeae905e, { 117, 64, 34 } },                 // csc100
    { 0xb0c02ec7639c752a, { 117, 63, 36 } },                 // csc200
    { 0x7cef91a4098b129e, { 76, 124, 204 } },                // cte100, cte200
    { 0xcf00294b5bbe04ac, { 71, 82, 144 } },                 // cte102
    { 0xdf306f2de74ea5fb, { 234, 239, 244 } },               // euc100
    { 0x590df41aad9bc6b7, { 218, 229, 250 } },               // euc200
    { 0xa630b72f3269025b, { 189, 165, 218 } },               // euc204
    { 0x57517a95b60d0b8d, { 23, 27, 34 } },                  // evt041
    { 0x94231efaa6d6521b, { 140, 228, 243 } },               // evt121, pam000, pam001
    { 0x63eac110710bbd42, { 255, 255, 166 } },               // fig000
    { 0xab24577085d0036a, { 200, 191, 231 } },               // ghz100
    { 0x308f8335b0308380, { 59, 73, 179 } },                 // ghz103
    { 0xb7a2ddeb23d3f789, { 0, 58, 105 } },                  // ghz104
    { 0xb281a7ad61dbf60b, { 145, 191, 255, 81, 181, 255 } }, // ghz200
    { 0xb42e0967e6fc9bac, { 136, 206, 223 } },               // pla100, pla200, pla205
    { 0xed11fd7b4d29de26, { 77, 88, 117 } },                 // pla204
    { 0xc59175a5f51dd8bd, { 11, 14, 93 } },                  // sph100
    { 0x61fabe09c63f8293, { 76, 88, 179 } },                 // sph101
    { 0x7a16b76d55221a78, { 0, 20, 116 } },                  // sph200
    { 0xffba3dbd0480fdfe, { 80, 91, 97 } },                  // ssh100
    { 0x75db373553a02a01, { 62, 29, 24 } },                  // ssh103
    { 0x4e15efedfbab15ec, { 125, 169, 203 } },               // ssh200
    { 0x30415167ec9b86ff, { 13, 10, 45 } },                  // ssh201
    { 0x65cc4d5ab937b17a, { 128, 169, 23 } },                // ssh205
    { 0x616a84fd9878922e, { 123, 211, 228 } },               // ssz100
    { 0x5e1cd647968bdbda, { 50, 51, 93 } },                  // ssz103
    { 0x9abdca7d29f70486, { 123, 203, 217 } },               // ssz200

    // Bob-omb Battlefield
    { 0xeac7b565235a67b3, { 63, 117, 214 } }, // ghz200

    // Camelot Castle
    { 0xba6a410f61762bc3, { 255, 255, 255 } }, // ghz200

    // Crashed Cove
    { 0xfcea54f15d3a3d8a, { 103, 129, 141 } }, // ghz200

    // Cyberspace Green Hill 1-1
    { 0x155391c7d437ba48, { 90, 141, 239 } }, // ghz200

    // Forces Metropolis
    { 0xf71cd2ef0825c0c, { 63, 93, 252 } }, // ghz200

    // Game Land Version 2.0
    { 0x15c73b567289a022, { 125, 187, 199 } }, // ghz200

    // Grand Metropolis
    { 0xe233121c4cde2788, { 52, 104, 171 } }, // ssz200

    // Lost Valley
    { 0x17c2d60f55303850, { 214, 232, 252 } }, // ghz200

    // [REDACTED]
    { 0xad1680e272c6fa71, { 147, 65, 0, 145, 134, 0 } }, // ghz200

    // Sandwich Zone
    { 0xea88a16844ad6117, { 255, 232, 180 } }, // ghz200

    // Seaside Hill Remake
    { 0x520e90cc2783e9f8, { 125, 169, 203 } }, // ssh200

    // Shivery Mountainsides
    { 0x998842f8fb7bcbfb, { 26, 26, 107 } }, // pam000
    { 0x115b84e6f466140d, { 63, 87, 255 } }, // ghz200

    // Sky Rail
    { 0x644673bd8eca9f9c, { 255, 255, 255 } }, // ssz200

    // Sonic Adventure Generations
    { 0x911ae3ad2b10f44e, { 52, 62, 77 } }, // ghz200
    { 0xc4f2d4a6278ddad6, { 215, 210, 185 } }, // cpz200
    { 0x963d2737a1fbe3a8, { 224, 253, 251 } }, // ssz200
    { 0x2f251e0135a5f8dd, { 180, 175, 180 } }, // euc200

    // Sonic Melponterations
    { 0xc1adebeb92051182, { 191, 187, 191 } }, // ghz200

    // Sonic Unleashed Wii Windmill Isle Act 1
    { 0xe59cb26b78b27c20, { 128, 127, 149 } }, // ghz200

    // Sony Hawk's Pro Skater
    { 0x1bdcdd0aea14b5f4, { 93, 49, 83 } }, // cte100

    // Spiral Reef
    { 0x93ad55ddbca4eb20, { 63, 150, 185 } }, // ghz200

    // Splash Hill Zone
    { 0xf926ad74fddc9174, { 255, 255, 255 } }, // ghz200

    // STH2006 Project Demo 5 Mission Extravaganza
    { 0x408cceb09378b5a7, { 171, 183, 255 } }, // ghz200
    // TODO ghz102
    { 0x2c3c74dd22156fb5, { 255, 158, 118 } }, // sph200
    { 0x330b89cd614d348f, { 225, 225, 246 } }, // csc200
    { 0x2924a236e178356a, { 91, 105, 111 } }, // euc200
    // TODO pam000

    // Titanic Timber
    { 0xaeec99b8e445961a, { 107, 161, 133 } }, // ghz200

    // Water Palace
    { 0xaf1c06ec1a7250a0, { 58, 90, 205 } }, // ghz200, pam000

    // Wave Ocean
    { 0x98a72de6a1476a01, { 255, 255, 255 } }, // pam000
    { 0xce8d6b9a3da7c45d, { 255, 255, 255 } }, // ghz200
};

bool EnvironmentColor::get(const GlobalsPS& globalsPS, float* skyColor, float* groundColor)
{
    XXH64_hash_t hash = 0;

    hash = XXH64(globalsPS.floatConstants[10], 12, hash);
    hash = XXH64(globalsPS.floatConstants[36], 12, hash);

    const auto pair = s_environmentColors.find(hash);

    if (pair != s_environmentColors.end() && (pair->second[0] != 0 || pair->second[1] != 0 || pair->second[2] != 0))
    {
        skyColor[0] = static_cast<float>(pair->second[0]) / 255.0f;
        skyColor[1] = static_cast<float>(pair->second[1]) / 255.0f;
        skyColor[2] = static_cast<float>(pair->second[2]) / 255.0f;

        if (pair->second[3] != 0 || pair->second[4] != 0 || pair->second[5] != 0)
        {
            groundColor[0] = static_cast<float>(pair->second[3]) / 255.0f;
            groundColor[1] = static_cast<float>(pair->second[4]) / 255.0f;
            groundColor[2] = static_cast<float>(pair->second[5]) / 255.0f;
        }
        else
        {
            groundColor[0] = skyColor[0];
            groundColor[1] = skyColor[1];
            groundColor[2] = skyColor[2];
        }

        return true;
    }

    return false;
}
