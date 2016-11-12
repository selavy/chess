#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

uint64_t clear_mask[65];
uint64_t set_mask[65];
const int lower_n = 16;
const int lower_b = 1;

#  define SetMask(a)             (set_mask[a])
#  define ClearMask(a)           (clear_mask[a])
const uint64_t magic_rook[64] = {
  0x0080001020400080ull, 0x0040001000200040ull, 0x0080081000200080ull,
  0x0080040800100080ull, 0x0080020400080080ull, 0x0080010200040080ull,
  0x0080008001000200ull, 0x0080002040800100ull, 0x0000800020400080ull,
  0x0000400020005000ull, 0x0000801000200080ull, 0x0000800800100080ull,
  0x0000800400080080ull, 0x0000800200040080ull, 0x0000800100020080ull,
  0x0000800040800100ull, 0x0000208000400080ull, 0x0000404000201000ull,
  0x0000808010002000ull, 0x0000808008001000ull, 0x0000808004000800ull,
  0x0000808002000400ull, 0x0000010100020004ull, 0x0000020000408104ull,
  0x0000208080004000ull, 0x0000200040005000ull, 0x0000100080200080ull,
  0x0000080080100080ull, 0x0000040080080080ull, 0x0000020080040080ull,
  0x0000010080800200ull, 0x0000800080004100ull, 0x0000204000800080ull,
  0x0000200040401000ull, 0x0000100080802000ull, 0x0000080080801000ull,
  0x0000040080800800ull, 0x0000020080800400ull, 0x0000020001010004ull,
  0x0000800040800100ull, 0x0000204000808000ull, 0x0000200040008080ull,
  0x0000100020008080ull, 0x0000080010008080ull, 0x0000040008008080ull,
  0x0000020004008080ull, 0x0000010002008080ull, 0x0000004081020004ull,
  0x0000204000800080ull, 0x0000200040008080ull, 0x0000100020008080ull,
  0x0000080010008080ull, 0x0000040008008080ull, 0x0000020004008080ull,
  0x0000800100020080ull, 0x0000800041000080ull, 0x00FFFCDDFCED714Aull,
  0x007FFCDDFCED714Aull, 0x003FFFCDFFD88096ull, 0x0000040810002101ull,
  0x0001000204080011ull, 0x0001000204000801ull, 0x0001000082000401ull,
  0x0001FFFAABFAD1A2ull
};
const uint64_t magic_rook_mask[64] = {
  0x000101010101017Eull, 0x000202020202027Cull, 0x000404040404047Aull,
  0x0008080808080876ull, 0x001010101010106Eull, 0x002020202020205Eull,
  0x004040404040403Eull, 0x008080808080807Eull, 0x0001010101017E00ull,
  0x0002020202027C00ull, 0x0004040404047A00ull, 0x0008080808087600ull,
  0x0010101010106E00ull, 0x0020202020205E00ull, 0x0040404040403E00ull,
  0x0080808080807E00ull, 0x00010101017E0100ull, 0x00020202027C0200ull,
  0x00040404047A0400ull, 0x0008080808760800ull, 0x00101010106E1000ull,
  0x00202020205E2000ull, 0x00404040403E4000ull, 0x00808080807E8000ull,
  0x000101017E010100ull, 0x000202027C020200ull, 0x000404047A040400ull,
  0x0008080876080800ull, 0x001010106E101000ull, 0x002020205E202000ull,
  0x004040403E404000ull, 0x008080807E808000ull, 0x0001017E01010100ull,
  0x0002027C02020200ull, 0x0004047A04040400ull, 0x0008087608080800ull,
  0x0010106E10101000ull, 0x0020205E20202000ull, 0x0040403E40404000ull,
  0x0080807E80808000ull, 0x00017E0101010100ull, 0x00027C0202020200ull,
  0x00047A0404040400ull, 0x0008760808080800ull, 0x00106E1010101000ull,
  0x00205E2020202000ull, 0x00403E4040404000ull, 0x00807E8080808000ull,
  0x007E010101010100ull, 0x007C020202020200ull, 0x007A040404040400ull,
  0x0076080808080800ull, 0x006E101010101000ull, 0x005E202020202000ull,
  0x003E404040404000ull, 0x007E808080808000ull, 0x7E01010101010100ull,
  0x7C02020202020200ull, 0x7A04040404040400ull, 0x7608080808080800ull,
  0x6E10101010101000ull, 0x5E20202020202000ull, 0x3E40404040404000ull,
  0x7E80808080808000ull
};
const unsigned magic_rook_shift[64] = {
  52, 53, 53, 53, 53, 53, 53, 52,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 53, 53, 53, 53, 53
};
uint64_t magic_rook_table[102400];
uint64_t *magic_rook_indices[64] = {
  magic_rook_table + 86016, magic_rook_table + 73728,
  magic_rook_table + 36864, magic_rook_table + 43008,
  magic_rook_table + 47104, magic_rook_table + 51200,
  magic_rook_table + 77824, magic_rook_table + 94208,
  magic_rook_table + 69632, magic_rook_table + 32768,
  magic_rook_table + 38912, magic_rook_table + 10240,
  magic_rook_table + 14336, magic_rook_table + 53248,
  magic_rook_table + 57344, magic_rook_table + 81920,
  magic_rook_table + 24576, magic_rook_table + 33792,
  magic_rook_table + 6144, magic_rook_table + 11264,
  magic_rook_table + 15360, magic_rook_table + 18432,
  magic_rook_table + 58368, magic_rook_table + 61440,
  magic_rook_table + 26624, magic_rook_table + 4096,
  magic_rook_table + 7168, magic_rook_table + 0,
  magic_rook_table + 2048, magic_rook_table + 19456,
  magic_rook_table + 22528, magic_rook_table + 63488,
  magic_rook_table + 28672, magic_rook_table + 5120,
  magic_rook_table + 8192, magic_rook_table + 1024,
  magic_rook_table + 3072, magic_rook_table + 20480,
  magic_rook_table + 23552, magic_rook_table + 65536,
  magic_rook_table + 30720, magic_rook_table + 34816,
  magic_rook_table + 9216, magic_rook_table + 12288,
  magic_rook_table + 16384, magic_rook_table + 21504,
  magic_rook_table + 59392, magic_rook_table + 67584,
  magic_rook_table + 71680, magic_rook_table + 35840,
  magic_rook_table + 39936, magic_rook_table + 13312,
  magic_rook_table + 17408, magic_rook_table + 54272,
  magic_rook_table + 60416, magic_rook_table + 83968,
  magic_rook_table + 90112, magic_rook_table + 75776,
  magic_rook_table + 40960, magic_rook_table + 45056,
  magic_rook_table + 49152, magic_rook_table + 55296,
  magic_rook_table + 79872, magic_rook_table + 98304
};

const uint64_t magic_bishop[64] = {
  0x0002020202020200ull, 0x0002020202020000ull, 0x0004010202000000ull,
  0x0004040080000000ull, 0x0001104000000000ull, 0x0000821040000000ull,
  0x0000410410400000ull, 0x0000104104104000ull, 0x0000040404040400ull,
  0x0000020202020200ull, 0x0000040102020000ull, 0x0000040400800000ull,
  0x0000011040000000ull, 0x0000008210400000ull, 0x0000004104104000ull,
  0x0000002082082000ull, 0x0004000808080800ull, 0x0002000404040400ull,
  0x0001000202020200ull, 0x0000800802004000ull, 0x0000800400A00000ull,
  0x0000200100884000ull, 0x0000400082082000ull, 0x0000200041041000ull,
  0x0002080010101000ull, 0x0001040008080800ull, 0x0000208004010400ull,
  0x0000404004010200ull, 0x0000840000802000ull, 0x0000404002011000ull,
  0x0000808001041000ull, 0x0000404000820800ull, 0x0001041000202000ull,
  0x0000820800101000ull, 0x0000104400080800ull, 0x0000020080080080ull,
  0x0000404040040100ull, 0x0000808100020100ull, 0x0001010100020800ull,
  0x0000808080010400ull, 0x0000820820004000ull, 0x0000410410002000ull,
  0x0000082088001000ull, 0x0000002011000800ull, 0x0000080100400400ull,
  0x0001010101000200ull, 0x0002020202000400ull, 0x0001010101000200ull,
  0x0000410410400000ull, 0x0000208208200000ull, 0x0000002084100000ull,
  0x0000000020880000ull, 0x0000001002020000ull, 0x0000040408020000ull,
  0x0004040404040000ull, 0x0002020202020000ull, 0x0000104104104000ull,
  0x0000002082082000ull, 0x0000000020841000ull, 0x0000000000208800ull,
  0x0000000010020200ull, 0x0000000404080200ull, 0x0000040404040400ull,
  0x0002020202020200ull
};
const uint64_t magic_bishop_mask[64] = {
  0x0040201008040200ull, 0x0000402010080400ull, 0x0000004020100A00ull,
  0x0000000040221400ull, 0x0000000002442800ull, 0x0000000204085000ull,
  0x0000020408102000ull, 0x0002040810204000ull, 0x0020100804020000ull,
  0x0040201008040000ull, 0x00004020100A0000ull, 0x0000004022140000ull,
  0x0000000244280000ull, 0x0000020408500000ull, 0x0002040810200000ull,
  0x0004081020400000ull, 0x0010080402000200ull, 0x0020100804000400ull,
  0x004020100A000A00ull, 0x0000402214001400ull, 0x0000024428002800ull,
  0x0002040850005000ull, 0x0004081020002000ull, 0x0008102040004000ull,
  0x0008040200020400ull, 0x0010080400040800ull, 0x0020100A000A1000ull,
  0x0040221400142200ull, 0x0002442800284400ull, 0x0004085000500800ull,
  0x0008102000201000ull, 0x0010204000402000ull, 0x0004020002040800ull,
  0x0008040004081000ull, 0x00100A000A102000ull, 0x0022140014224000ull,
  0x0044280028440200ull, 0x0008500050080400ull, 0x0010200020100800ull,
  0x0020400040201000ull, 0x0002000204081000ull, 0x0004000408102000ull,
  0x000A000A10204000ull, 0x0014001422400000ull, 0x0028002844020000ull,
  0x0050005008040200ull, 0x0020002010080400ull, 0x0040004020100800ull,
  0x0000020408102000ull, 0x0000040810204000ull, 0x00000A1020400000ull,
  0x0000142240000000ull, 0x0000284402000000ull, 0x0000500804020000ull,
  0x0000201008040200ull, 0x0000402010080400ull, 0x0002040810204000ull,
  0x0004081020400000ull, 0x000A102040000000ull, 0x0014224000000000ull,
  0x0028440200000000ull, 0x0050080402000000ull, 0x0020100804020000ull,
  0x0040201008040200ull
};
const unsigned magic_bishop_shift[64] = {
  58, 59, 59, 59, 59, 59, 59, 58,
  59, 59, 59, 59, 59, 59, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 59, 59, 59, 59, 59, 59,
  58, 59, 59, 59, 59, 59, 59, 58
};
uint64_t magic_bishop_table[5248];
uint64_t *magic_bishop_indices[64] = {
  magic_bishop_table + 4992, magic_bishop_table + 2624,
  magic_bishop_table + 256, magic_bishop_table + 896,
  magic_bishop_table + 1280, magic_bishop_table + 1664,
  magic_bishop_table + 4800, magic_bishop_table + 5120,
  magic_bishop_table + 2560, magic_bishop_table + 2656,
  magic_bishop_table + 288, magic_bishop_table + 928,
  magic_bishop_table + 1312, magic_bishop_table + 1696,
  magic_bishop_table + 4832, magic_bishop_table + 4928,
  magic_bishop_table + 0, magic_bishop_table + 128,
  magic_bishop_table + 320, magic_bishop_table + 960,
  magic_bishop_table + 1344, magic_bishop_table + 1728,
  magic_bishop_table + 2304, magic_bishop_table + 2432,
  magic_bishop_table + 32, magic_bishop_table + 160,
  magic_bishop_table + 448, magic_bishop_table + 2752,
  magic_bishop_table + 3776, magic_bishop_table + 1856,
  magic_bishop_table + 2336, magic_bishop_table + 2464,
  magic_bishop_table + 64, magic_bishop_table + 192,
  magic_bishop_table + 576, magic_bishop_table + 3264,
  magic_bishop_table + 4288, magic_bishop_table + 1984,
  magic_bishop_table + 2368, magic_bishop_table + 2496,
  magic_bishop_table + 96, magic_bishop_table + 224,
  magic_bishop_table + 704, magic_bishop_table + 1088,
  magic_bishop_table + 1472, magic_bishop_table + 2112,
  magic_bishop_table + 2400, magic_bishop_table + 2528,
  magic_bishop_table + 2592, magic_bishop_table + 2688,
  magic_bishop_table + 832, magic_bishop_table + 1216,
  magic_bishop_table + 1600, magic_bishop_table + 2240,
  magic_bishop_table + 4864, magic_bishop_table + 4960,
  magic_bishop_table + 5056, magic_bishop_table + 2720,
  magic_bishop_table + 864, magic_bishop_table + 1248,
  magic_bishop_table + 1632, magic_bishop_table + 2272,
  magic_bishop_table + 4896, magic_bishop_table + 5184
};

#  define BishopAttacks(square, occ) *(magic_bishop_indices[square]+((((occ)&magic_bishop_mask[square])*magic_bishop[square])>>magic_bishop_shift[square]))

uint64_t InitializeMagicOccupied(int *squares, int numSquares,
    uint64_t linoccupied) {
  int i;
  uint64_t ret = 0;

  for (i = 0; i < numSquares; i++)
    if (linoccupied & (uint64_t) 1 << i)
      ret |= (uint64_t) 1 << squares[i];
  return ret;
}

uint64_t InitializeMagicRook(int square, uint64_t occupied) {
  uint64_t ret = 0;
  uint64_t abit;
  uint64_t rowbits = (uint64_t) 0xFF << 8 * (square / 8);

  abit = (uint64_t) 1 << square;
  do {
    abit <<= 8;
    ret |= abit;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  do {
    abit >>= 8;
    ret |= abit;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  do {
    abit <<= 1;
    if (abit & rowbits)
      ret |= abit;
    else
      break;
  } while (!(abit & occupied));
  abit = (uint64_t) 1 << square;
  do {
    abit >>= 1;
    if (abit & rowbits)
      ret |= abit;
    else
      break;
  } while (!(abit & occupied));
  return ret;
}
uint64_t InitializeMagicBishop(int square, uint64_t occupied) {
  uint64_t ret = 0;
  uint64_t abit;
  uint64_t abit2;
  uint64_t rowbits = (uint64_t) 0xFF << 8 * (square / 8);

  abit = (uint64_t) 1 << square;
  abit2 = abit;
  do {
    abit <<= 8 - 1;
    abit2 >>= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  abit2 = abit;
  do {
    abit <<= 8 + 1;
    abit2 <<= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  abit2 = abit;
  do {
    abit >>= 8 - 1;
    abit2 <<= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  abit = (uint64_t) 1 << square;
  abit2 = abit;
  do {
    abit >>= 8 + 1;
    abit2 >>= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  return ret;
}

// taken from crafty, `init.c` and `data.c`
void InitializeMagic(void) {
    int i;//, j, m;
  int initmagicmoves_bitpos64_database[64] = {
    63, 0, 58, 1, 59, 47, 53, 2,
    60, 39, 48, 27, 54, 33, 42, 3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22, 4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16, 9, 12,
    44, 24, 15, 8, 23, 7, 6, 5
  };
/*
 Bishop attacks and mobility
 */
  for (i = 0; i < 64; i++) {
    int squares[64];
    int numsquares = 0;
    uint64_t temp = magic_bishop_mask[i];

    while (temp) {
      uint64_t abit = temp & -temp;
      squares[numsquares++] =
          initmagicmoves_bitpos64_database[(abit *
              0x07EDD5E59A4E28C2ull) >> 58];
      temp ^= abit;
    }
    for (temp = 0; temp < (uint64_t) 1 << numsquares; temp++) {
      uint64_t moves;
      uint64_t tempoccupied =
          InitializeMagicOccupied(squares, numsquares, temp);
      moves = InitializeMagicBishop(i, tempoccupied);
      *(magic_bishop_indices[i] +
          (tempoccupied * magic_bishop[i] >> magic_bishop_shift[i])) = moves;
    }
  }

/*
 Rook attacks and mobility
 */
  for (i = 0; i < 64; i++) {
    int squares[64];
    int numsquares = 0;
    uint64_t temp = magic_rook_mask[i];

    while (temp) {
      uint64_t abit = temp & -temp;

      squares[numsquares++] =
          initmagicmoves_bitpos64_database[(abit *
              0x07EDD5E59A4E28C2ull) >> 58];
      temp ^= abit;
    }
    for (temp = 0; temp < (uint64_t) 1 << numsquares; temp++) {
      uint64_t tempoccupied =
          InitializeMagicOccupied(squares, numsquares, temp);
      uint64_t moves = InitializeMagicRook(i, tempoccupied);
      *(magic_rook_indices[i] +
          (tempoccupied * magic_rook[i] >> magic_rook_shift[i])) = moves;
      moves |= SetMask(i);
      /* m = -1; */
      /* for (j = 0; j < 4; j++) */
      /*   m += PopCnt(moves & mobility_mask_r[j]) * mobility_score_r[j]; */
      /* *(magic_rook_mobility_indices[i] + */
      /*     (tempoccupied * magic_rook[i] >> magic_rook_shift[i])) = */
      /*     mob_curve_r[m]; */
    }
  }  
}

static const uint64_t knight_attacks[64] = {
    132096ULL,329728ULL,659712ULL,1319424ULL,2638848ULL,5277696ULL,10489856ULL,4202496ULL,33816580ULL,84410376ULL,168886289ULL,337772578ULL,675545156ULL,1351090312ULL,2685403152ULL,1075839008ULL,8657044482ULL,21609056261ULL,43234889994ULL,86469779988ULL,172939559976ULL,345879119952ULL,687463207072ULL,275414786112ULL,2216203387392ULL,5531918402816ULL,11068131838464ULL,22136263676928ULL,44272527353856ULL,88545054707712ULL,175990581010432ULL,70506185244672ULL,567348067172352ULL,1416171111120896ULL,2833441750646784ULL,5666883501293568ULL,11333767002587136ULL,22667534005174272ULL,45053588738670592ULL,18049583422636032ULL,145241105196122112ULL,362539804446949376ULL,725361088165576704ULL,1450722176331153408ULL,2901444352662306816ULL,5802888705324613632ULL,11533718717099671552ULL,4620693356194824192ULL,288234782788157440ULL,576469569871282176ULL,1224997833292120064ULL,2449995666584240128ULL,4899991333168480256ULL,9799982666336960512ULL,1152939783987658752ULL,2305878468463689728ULL,1128098930098176ULL,2257297371824128ULL,4796069720358912ULL,9592139440717824ULL,19184278881435648ULL,38368557762871296ULL,4679521487814656ULL,9077567998918656ULL
};
static const uint64_t king_attacks[64] = { 770ULL,1797ULL,3594ULL,7188ULL,14376ULL,28752ULL,57504ULL,49216ULL,197123ULL,460039ULL,920078ULL,1840156ULL,3680312ULL,7360624ULL,14721248ULL,12599488ULL,50463488ULL,117769984ULL,235539968ULL,471079936ULL,942159872ULL,1884319744ULL,3768639488ULL,3225468928ULL,12918652928ULL,30149115904ULL,60298231808ULL,120596463616ULL,241192927232ULL,482385854464ULL,964771708928ULL,825720045568ULL,3307175149568ULL,7718173671424ULL,15436347342848ULL,30872694685696ULL,61745389371392ULL,123490778742784ULL,246981557485568ULL,211384331665408ULL,846636838289408ULL,1975852459884544ULL,3951704919769088ULL,7903409839538176ULL,15806819679076352ULL,31613639358152704ULL,63227278716305408ULL,54114388906344448ULL,216739030602088448ULL,505818229730443264ULL,1011636459460886528ULL,2023272918921773056ULL,4046545837843546112ULL,8093091675687092224ULL,16186183351374184448ULL,13853283560024178688ULL,144959613005987840ULL,362258295026614272ULL,724516590053228544ULL,1449033180106457088ULL,2898066360212914176ULL,5796132720425828352ULL,11592265440851656704ULL,4665729213955833856ULL };

int main(int argc, char **argv) {
    InitializeMagic();
    FILE *fp = fopen("magic_tables.c", "w");
    if (!fp) {
        fputs("Failed to open \"magic_tables.c\"", stderr);
        exit(EXIT_FAILURE);
    }
    FILE *hdr = fopen("magic_tables.h", "w");
    if (!hdr) {
        fclose(fp);
        fputs("Failed to open \"magic_tables.h\"", stderr);
        exit(EXIT_FAILURE);
    }
    fputs("#ifndef MAGIC_TABLES__H_\n", hdr);
    fputs("#define MAGIC_TABLES__H_\n\n", hdr);
    fputs("//\n// Generated by `generate_magic_tables.c`\n//\n\n", hdr);    
    fputs("#include <stdint.h>\n\n", hdr);
    fputs("extern const uint64_t _knight_attacks[64];\n", hdr);
    fputs("extern const uint64_t _king_attacks[64];\n", hdr);
    fputs("extern const uint64_t magic_bishop[64];\n", hdr);
    fputs("extern const uint64_t magic_bishop_mask[64];\n", hdr);
    fputs("extern const uint32_t magic_bishop_shift[64];\n", hdr);
    fputs("extern const uint64_t magic_bishop_table[5428];\n", hdr);
    fputs("extern const uint64_t *magic_bishop_indices[64];\n", hdr);
    fputs("extern const uint64_t magic_rook[64];\n", hdr);
    fputs("extern const uint64_t magic_rook_mask[64];\n", hdr);
    fputs("extern const unsigned magic_rook_shift[64];\n", hdr);
    fputs("extern const uint64_t magic_rook_table[102400];\n", hdr);
    fputs("extern const uint64_t *magic_rook_indices[64];\n", hdr);
    fputs("extern const uint64_t slide_attacks[64];\n", hdr);
    fputs("extern const uint64_t diagl_attacks[64];\n", hdr);
    fputs("extern const uint64_t wpawn_attacks[64];\n", hdr);
    fputs("extern const uint64_t bpawn_attacks[64];\n", hdr);        
    fputs("\n", hdr);
    fputs("#define knight_attacks(sq) _knight_attacks[sq]\n", hdr);
    fputs("#define king_attacks(sq)   _king_attacks[sq]\n", hdr);
    fputs("#define bishop_attacks(square, occ)                                     \\\n"
          "    *(magic_bishop_indices[square] +                                    \\\n"
          "      ((((occ) & magic_bishop_mask[square]) * magic_bishop[square])     \\\n"
          "       >> magic_bishop_shift[square]))\n", hdr);
    fputs("#define rook_attacks(square, occ) *(magic_rook_indices[square]+((((occ)&magic_rook_mask[square])*magic_rook[square])>>magic_rook_shift[square]))\n", hdr);
    fputs("#define queen_attacks(square, occ) (bishop_attacks(square, occ) | rook_attacks(square, occ))\n", hdr);
    fputs("\n", hdr);
    fputs("#endif // MAGIC_TABLES__H_\n", hdr);

    fprintf(fp, "#include \"magic_tables.h\"\n\n");
    fprintf(fp, "//\n");
    fprintf(fp, "//   Idea and table generation code shamelessly taken from Crafty\n");
    fprintf(fp, "//   I did at least generate my own knight, pawn, and king attacks ;)\n");
    fprintf(fp, "//\n");
    fprintf(fp, "\n");

    fprintf(fp, "const uint64_t _knight_attacks[64] = {\n");
    for (int i = 0; i < 62; i += 2) {
        fprintf(fp, "    0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull,\n", knight_attacks[i], knight_attacks[i+1]);
    }
    fprintf(fp, "    0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull\n", knight_attacks[62], knight_attacks[63]);
    fprintf(fp, "};\n");
    
    fprintf(fp, "const uint64_t _king_attacks[64] = {\n");
    for (int i = 0; i < 62; i += 2) {
        fprintf(fp, "    0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull,\n", king_attacks[i], king_attacks[i+1]);
    }
    fprintf(fp, "    0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull\n", king_attacks[62], king_attacks[63]);
    fprintf(fp, "};\n");
    
    fprintf(fp, "const uint64_t magic_bishop[64] = {\n"
"    0x0002020202020200ull, 0x0002020202020000ull, 0x0004010202000000ull,\n"
"    0x0004040080000000ull, 0x0001104000000000ull, 0x0000821040000000ull,\n"
"    0x0000410410400000ull, 0x0000104104104000ull, 0x0000040404040400ull,\n"
"    0x0000020202020200ull, 0x0000040102020000ull, 0x0000040400800000ull,\n"
"    0x0000011040000000ull, 0x0000008210400000ull, 0x0000004104104000ull,\n"
"    0x0000002082082000ull, 0x0004000808080800ull, 0x0002000404040400ull,\n"
"    0x0001000202020200ull, 0x0000800802004000ull, 0x0000800400A00000ull,\n"
"    0x0000200100884000ull, 0x0000400082082000ull, 0x0000200041041000ull,\n"
"    0x0002080010101000ull, 0x0001040008080800ull, 0x0000208004010400ull,\n"
"    0x0000404004010200ull, 0x0000840000802000ull, 0x0000404002011000ull,\n"
"    0x0000808001041000ull, 0x0000404000820800ull, 0x0001041000202000ull,\n"
"    0x0000820800101000ull, 0x0000104400080800ull, 0x0000020080080080ull,\n"
"    0x0000404040040100ull, 0x0000808100020100ull, 0x0001010100020800ull,\n"
"    0x0000808080010400ull, 0x0000820820004000ull, 0x0000410410002000ull,\n"
"    0x0000082088001000ull, 0x0000002011000800ull, 0x0000080100400400ull,\n"
"    0x0001010101000200ull, 0x0002020202000400ull, 0x0001010101000200ull,\n"
"    0x0000410410400000ull, 0x0000208208200000ull, 0x0000002084100000ull,\n"
"    0x0000000020880000ull, 0x0000001002020000ull, 0x0000040408020000ull,\n"
"    0x0004040404040000ull, 0x0002020202020000ull, 0x0000104104104000ull,\n"
"    0x0000002082082000ull, 0x0000000020841000ull, 0x0000000000208800ull,\n"
"    0x0000000010020200ull, 0x0000000404080200ull, 0x0000040404040400ull,\n"
"    0x0002020202020200ull\n"
"};\n"
"const uint64_t magic_bishop_mask[64] = {\n"
"    0x0040201008040200ull, 0x0000402010080400ull, 0x0000004020100A00ull,\n"
"    0x0000000040221400ull, 0x0000000002442800ull, 0x0000000204085000ull,\n"
"    0x0000020408102000ull, 0x0002040810204000ull, 0x0020100804020000ull,\n"
"    0x0040201008040000ull, 0x00004020100A0000ull, 0x0000004022140000ull,\n"
"    0x0000000244280000ull, 0x0000020408500000ull, 0x0002040810200000ull,\n"
"    0x0004081020400000ull, 0x0010080402000200ull, 0x0020100804000400ull,\n"
"    0x004020100A000A00ull, 0x0000402214001400ull, 0x0000024428002800ull,\n"
"    0x0002040850005000ull, 0x0004081020002000ull, 0x0008102040004000ull,\n"
"    0x0008040200020400ull, 0x0010080400040800ull, 0x0020100A000A1000ull,\n"
"    0x0040221400142200ull, 0x0002442800284400ull, 0x0004085000500800ull,\n"
"    0x0008102000201000ull, 0x0010204000402000ull, 0x0004020002040800ull,\n"
"    0x0008040004081000ull, 0x00100A000A102000ull, 0x0022140014224000ull,\n"
"    0x0044280028440200ull, 0x0008500050080400ull, 0x0010200020100800ull,\n"
"    0x0020400040201000ull, 0x0002000204081000ull, 0x0004000408102000ull,\n"
"    0x000A000A10204000ull, 0x0014001422400000ull, 0x0028002844020000ull,\n"
"    0x0050005008040200ull, 0x0020002010080400ull, 0x0040004020100800ull,\n"
"    0x0000020408102000ull, 0x0000040810204000ull, 0x00000A1020400000ull,\n"
"    0x0000142240000000ull, 0x0000284402000000ull, 0x0000500804020000ull,\n"
"    0x0000201008040200ull, 0x0000402010080400ull, 0x0002040810204000ull,\n"
"    0x0004081020400000ull, 0x000A102040000000ull, 0x0014224000000000ull,\n"
"    0x0028440200000000ull, 0x0050080402000000ull, 0x0020100804020000ull,\n"
"    0x0040201008040200ull\n"
"};\n"
"const unsigned magic_bishop_shift[64] = {\n"
"    58, 59, 59, 59, 59, 59, 59, 58,\n"
"    59, 59, 59, 59, 59, 59, 59, 59,\n"
"    59, 59, 57, 57, 57, 57, 59, 59,\n"
"    59, 59, 57, 55, 55, 57, 59, 59,\n"
"    59, 59, 57, 55, 55, 57, 59, 59,\n"
"    59, 59, 57, 57, 57, 57, 59, 59,\n"
"    59, 59, 59, 59, 59, 59, 59, 59,\n"
"    58, 59, 59, 59, 59, 59, 59, 58\n"
"};\n");
    
    fprintf(fp, "const uint64_t magic_bishop_table[5428] = {\n");
    for (int i = 0; i < (5248-2); i += 3) {
        fprintf(fp, "    0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull,\n",
               magic_bishop_table[i],
               magic_bishop_table[i+1],
               magic_bishop_table[i+2]
               );
    }
    fprintf(fp, "    0x%016" PRIX64 "ull\n", magic_bishop_table[5247]);
    fprintf(fp, "};\n");
    int mbi[64] = {
        4992, 2624,
        256, 896,
        1280, 1664,
        4800, 5120,
        2560, 2656,
        288, 928,
        1312, 1696,
        4832, 4928,
        0, 128,
        320, 960,
        1344, 1728,
        2304, 2432,
        32, 160,
        448, 2752,
        3776, 1856,
        2336, 2464,
        64, 192,
        576, 3264,
        4288, 1984,
        2368, 2496,
        96, 224,
        704, 1088,
        1472, 2112,
        2400, 2528,
        2592, 2688,
        832, 1216,
        1600, 2240,
        4864, 4960,
        5056, 2720,
        864, 1248,
        1632, 2272,
        4896, 5184
    };
    fputs("const uint64_t *magic_bishop_indices[64] = {\n", fp);
    for (int i = 0; i < 62; i += 2) {
        fprintf(fp, "    magic_bishop_table + 0x%04X, magic_bishop_table + 0x%04X,\n",
                mbi[i], mbi[i+1]);
    }
    fprintf(fp, "    magic_bishop_table + 0x%04X, magic_bishop_table + 0x%04X\n};\n",
            mbi[62], mbi[63]);
    
    // Rooks
    fputs("const uint64_t magic_rook[64] = {\n"
"    0x0080001020400080ull, 0x0040001000200040ull, 0x0080081000200080ull,\n"
"    0x0080040800100080ull, 0x0080020400080080ull, 0x0080010200040080ull,\n"
"    0x0080008001000200ull, 0x0080002040800100ull, 0x0000800020400080ull,\n"
"    0x0000400020005000ull, 0x0000801000200080ull, 0x0000800800100080ull,\n"
"    0x0000800400080080ull, 0x0000800200040080ull, 0x0000800100020080ull,\n"
"    0x0000800040800100ull, 0x0000208000400080ull, 0x0000404000201000ull,\n"
"    0x0000808010002000ull, 0x0000808008001000ull, 0x0000808004000800ull,\n"
"    0x0000808002000400ull, 0x0000010100020004ull, 0x0000020000408104ull,\n"
"    0x0000208080004000ull, 0x0000200040005000ull, 0x0000100080200080ull,\n"
"    0x0000080080100080ull, 0x0000040080080080ull, 0x0000020080040080ull,\n"
"    0x0000010080800200ull, 0x0000800080004100ull, 0x0000204000800080ull,\n"
"    0x0000200040401000ull, 0x0000100080802000ull, 0x0000080080801000ull,\n"
"    0x0000040080800800ull, 0x0000020080800400ull, 0x0000020001010004ull,\n"
"    0x0000800040800100ull, 0x0000204000808000ull, 0x0000200040008080ull,\n"
"    0x0000100020008080ull, 0x0000080010008080ull, 0x0000040008008080ull,\n"
"    0x0000020004008080ull, 0x0000010002008080ull, 0x0000004081020004ull,\n"
"    0x0000204000800080ull, 0x0000200040008080ull, 0x0000100020008080ull,\n"
"    0x0000080010008080ull, 0x0000040008008080ull, 0x0000020004008080ull,\n"
"    0x0000800100020080ull, 0x0000800041000080ull, 0x00FFFCDDFCED714Aull,\n"
"    0x007FFCDDFCED714Aull, 0x003FFFCDFFD88096ull, 0x0000040810002101ull,\n"
"    0x0001000204080011ull, 0x0001000204000801ull, 0x0001000082000401ull,\n"
"    0x0001FFFAABFAD1A2ull\n"
"};\n"
"const uint64_t magic_rook_mask[64] = {\n"
"    0x000101010101017Eull, 0x000202020202027Cull, 0x000404040404047Aull,\n"
"    0x0008080808080876ull, 0x001010101010106Eull, 0x002020202020205Eull,\n"
"    0x004040404040403Eull, 0x008080808080807Eull, 0x0001010101017E00ull,\n"
"    0x0002020202027C00ull, 0x0004040404047A00ull, 0x0008080808087600ull,\n"
"    0x0010101010106E00ull, 0x0020202020205E00ull, 0x0040404040403E00ull,\n"
"    0x0080808080807E00ull, 0x00010101017E0100ull, 0x00020202027C0200ull,\n"
"    0x00040404047A0400ull, 0x0008080808760800ull, 0x00101010106E1000ull,\n"
"    0x00202020205E2000ull, 0x00404040403E4000ull, 0x00808080807E8000ull,\n"
"    0x000101017E010100ull, 0x000202027C020200ull, 0x000404047A040400ull,\n"
"    0x0008080876080800ull, 0x001010106E101000ull, 0x002020205E202000ull,\n"
"    0x004040403E404000ull, 0x008080807E808000ull, 0x0001017E01010100ull,\n"
"    0x0002027C02020200ull, 0x0004047A04040400ull, 0x0008087608080800ull,\n"
"    0x0010106E10101000ull, 0x0020205E20202000ull, 0x0040403E40404000ull,\n"
"    0x0080807E80808000ull, 0x00017E0101010100ull, 0x00027C0202020200ull,\n"
"    0x00047A0404040400ull, 0x0008760808080800ull, 0x00106E1010101000ull,\n"
"    0x00205E2020202000ull, 0x00403E4040404000ull, 0x00807E8080808000ull,\n"
"    0x007E010101010100ull, 0x007C020202020200ull, 0x007A040404040400ull,\n"
"    0x0076080808080800ull, 0x006E101010101000ull, 0x005E202020202000ull,\n"
"    0x003E404040404000ull, 0x007E808080808000ull, 0x7E01010101010100ull,\n"
"    0x7C02020202020200ull, 0x7A04040404040400ull, 0x7608080808080800ull,\n"
"    0x6E10101010101000ull, 0x5E20202020202000ull, 0x3E40404040404000ull,\n"
"    0x7E80808080808000ull\n"
"};\n"
"const unsigned magic_rook_shift[64] = {\n"
"    52, 53, 53, 53, 53, 53, 53, 52,\n"
"    53, 54, 54, 54, 54, 54, 54, 53,\n"
"    53, 54, 54, 54, 54, 54, 54, 53,\n"
"    53, 54, 54, 54, 54, 54, 54, 53,\n"
"    53, 54, 54, 54, 54, 54, 54, 53,\n"
"    53, 54, 54, 54, 54, 54, 54, 53,\n"
"    53, 54, 54, 54, 54, 54, 54, 53,\n"
"    53, 54, 54, 53, 53, 53, 53, 53\n"
"};\n", fp);

    fprintf(fp, "const uint64_t magic_rook_table[102400] = {\n");
    for (int i = 0; i < (102400-4); i += 3) {
        fprintf(fp, "    0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull,\n",
               magic_rook_table[i],
               magic_rook_table[i+1],
               magic_rook_table[i+2]
               );
    }
    fprintf(fp, "    0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull, 0x%016" PRIX64 "ull\n",
            magic_rook_table[102397],
            magic_rook_table[102398],
            magic_rook_table[102399]
            );    
    fprintf(fp, "};\n");
    
    int mri[64] = {
        86016, 73728,
        36864, 43008,
        47104, 51200,
        77824, 94208,
        69632, 32768,
        38912, 10240,
        14336, 53248,
        57344, 81920,
        24576, 33792,
        6144, 11264,
        15360, 18432,
        58368, 61440,
        26624, 4096,
        7168, 0,
        2048, 19456,
        22528, 63488,
        28672, 5120,
        8192, 1024,
        3072, 20480,
        23552, 65536,
        30720, 34816,
        9216, 12288,
        16384, 21504,
        59392, 67584,
        71680, 35840,
        39936, 13312,
        17408, 54272,
        60416, 83968,
        90112, 75776,
        40960, 45056,
        49152, 55296,
        79872, 98304
    };
    fputs("const uint64_t *magic_rook_indices[64] = {\n", fp);
    for (int i = 0; i < 62; i += 2) {
        fprintf(fp, "    magic_rook_table + 0x%08X, magic_rook_table + 0x%08X,\n",
                mri[i], mri[i+1]);
    }
    fprintf(fp, "    magic_rook_table + 0x%08X, magic_rook_table + 0x%08X\n};\n",
            mri[62], mri[63]);


    uint64_t slide_attacks[64];
    uint64_t diagl_attacks[64];
    uint64_t wpawn_attacks[64];
    uint64_t bpawn_attacks[64];
    memset(&slide_attacks[0], 0, sizeof(slide_attacks[0]) * 64);
    memset(&diagl_attacks[0], 0, sizeof(diagl_attacks[0]) * 64);
    memset(&wpawn_attacks[0], 0, sizeof(wpawn_attacks[0]) * 64);
    memset(&bpawn_attacks[0], 0, sizeof(bpawn_attacks[0]) * 64);    

    int boff_r[4] = { -1, -1,  1,  1 };
    int boff_c[4] = { -1,  1, -1,  1 };
    int roff_r[4] = { -1,  1,  0,  0 };
    int roff_c[4] = {  0,  0, -1,  1 };

    int rank;
    int col;
    int sq;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            sq = r * 8 + c;
            for (int i = 1; i < 8; ++i) {
                for (int o = 0; o < 4; ++o) {
                    rank = r + boff_r[o]*i;
                    col  = c + boff_c[o]*i; 
                    if (rank >= 0 && rank <= 7 && col >= 0 && col <= 7) {
                        diagl_attacks[sq] |= ((uint64_t)1 << ((rank * 8) + col));
                    }
                    rank = r + roff_r[o]*i;
                    col  = c + roff_c[o]*i;
                    if (rank >= 0 && rank <= 7 && col >= 0 && col <= 7) {
                        slide_attacks[sq] |= ((uint64_t)1 << ((rank * 8) + col));
                    }
                }
            }

            // pawn attacks
            if (r != 0 && r != 7) { // 1st and 8th rank can't capture anything
                // capture left
                if (c != 0) { // not left column
                    wpawn_attacks[sq] |= (uint64_t)1 << (sq + 7);
                    bpawn_attacks[sq] |= (uint64_t)1 << (sq - 9);
                }
                // capture right
                if (c != 7) { // not right column
                    wpawn_attacks[sq] |= (uint64_t)1 << (sq + 9);
                    bpawn_attacks[sq] |= (uint64_t)1 << (sq - 7);
                }
            } else if (r == 0) {
                // capture left
                if (c != 0) { // not left column
                    wpawn_attacks[sq] |= (uint64_t)1 << (sq + 7);
                }
                // capture right
                if (c != 7) { // not right column
                    wpawn_attacks[sq] |= (uint64_t)1 << (sq + 9);
                }                
            } else if (r == 7) {
                // capture left
                if (c != 0) { // not left column
                    bpawn_attacks[sq] |= (uint64_t)1 << (sq - 9);
                }
                // capture right
                if (c != 7) { // not right column
                    bpawn_attacks[sq] |= (uint64_t)1 << (sq - 7);
                }                
            }
        }
    }
    
    fputs("const uint64_t slide_attacks[64] = {\n", fp);
    for (int i = 0; i < 60; i += 4) {
        fprintf(fp, "    0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ",\n",
                slide_attacks[i], slide_attacks[i+1],
                slide_attacks[i+2], slide_attacks[i+3]);
    }
    fprintf(fp, "    0x%08" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 "\n};\n",
            slide_attacks[60], slide_attacks[61],
            slide_attacks[62], slide_attacks[63]);
    
    fputs("const uint64_t diagl_attacks[64] = {\n", fp);
    for (int i = 0; i < 60; i += 4) {
        fprintf(fp, "    0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ",\n",
                diagl_attacks[i], diagl_attacks[i+1],
                diagl_attacks[i+2], diagl_attacks[i+3]);
    }
    fprintf(fp, "    0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 "\n};\n",
            diagl_attacks[60], diagl_attacks[61],
            diagl_attacks[62], diagl_attacks[63]);
    fputs("const uint64_t wpawn_attacks[64] = {\n", fp);
    for (int i = 0; i < 60; i += 4) {
        fprintf(fp, "    0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ",\n",
                wpawn_attacks[i], wpawn_attacks[i+1],
                wpawn_attacks[i+2], wpawn_attacks[i+3]);
    }
    fprintf(fp, "    0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 "\n};\n",
            wpawn_attacks[60], wpawn_attacks[61],
            wpawn_attacks[62], wpawn_attacks[63]);
    fputs("const uint64_t bpawn_attacks[64] = {\n", fp);
    for (int i = 0; i < 60; i += 4) {
        fprintf(fp, "    0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ",\n",
                bpawn_attacks[i], bpawn_attacks[i+1],
                bpawn_attacks[i+2], bpawn_attacks[i+3]);
    }
    fprintf(fp, "    0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 ", 0x%016" PRIX64 "\n};\n",
            bpawn_attacks[60], bpawn_attacks[61],
            bpawn_attacks[62], bpawn_attacks[63]);    
    
    
    
    fputs("\n\n", hdr);
    fclose(hdr);
    fclose(fp);
    
    return 0;
}
