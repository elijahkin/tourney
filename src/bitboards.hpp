#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

// A bitboard-based Chess move generator

// ================================ Core Types ================================

constexpr size_t kNumSquares = 64;
constexpr size_t kNumPieces = 6;
constexpr size_t kNumColors = 2;
constexpr size_t kMaxMoves = 218;

using Bitboard = uint64_t;
using SquareIndex = uint8_t;

enum class Piece : uint8_t { kPawn, kKnight, kBishop, kRook, kQueen, kKing };
enum class Color : uint8_t { kWhite, kBlack };

constexpr size_t idx(Color c) { return static_cast<size_t>(c); }
constexpr size_t idx(Piece p) { return static_cast<size_t>(p); }

struct State {
  std::array<std::array<Bitboard, kNumPieces>, kNumColors> bbs{};
  Color side_to_move = Color::kWhite;
  // TODO castling rights, en passant square
};

// ============================= Bitboard Helpers =============================

constexpr Bitboard kFileA = 0x0101010101010101;
constexpr Bitboard kFileB = kFileA << 1U;
constexpr Bitboard kFileG = kFileA << 6U;
constexpr Bitboard kFileH = kFileA << 7U;

// ============================== Attack Tables ===============================

constexpr Bitboard knight_attack(Bitboard bb) {
  return ((bb << 17U) & ~kFileA) |             // NNE
         ((bb << 15U) & ~kFileH) |             // NNW
         ((bb << 10U) & ~(kFileA | kFileB)) |  // ENE
         ((bb << 6U) & ~(kFileG | kFileH)) |   // WNW
         ((bb >> 17U) & ~kFileH) |             // SSW
         ((bb >> 15U) & ~kFileA) |             // SSE
         ((bb >> 10U) & ~(kFileG | kFileH)) |  // WSW
         ((bb >> 6U) & ~(kFileA | kFileB));    // ESE
}

constexpr auto kKnightAttacks = [] {
  std::array<Bitboard, kNumSquares> table{};
  for (SquareIndex sq = 0; sq < kNumSquares; ++sq) {
    table[sq] = knight_attack(Bitboard{1} << sq);
  }
  return table;
}();

constexpr Bitboard king_attack(Bitboard bb) {
  return (bb << 8U) | (bb >> 8U) |                          // N, S
         ((bb << 1U) & ~kFileA) | ((bb >> 1U) & ~kFileH) |  // E, W
         ((bb << 9U) & ~kFileA) | ((bb << 7U) & ~kFileH) |  // NE, NW
         ((bb >> 7U) & ~kFileA) | ((bb >> 9U) & ~kFileH);   // SE, SW
}

constexpr auto kKingAttacks = [] {
  std::array<Bitboard, kNumSquares> table{};
  for (SquareIndex sq = 0; sq < kNumSquares; ++sq) {
    table[sq] = king_attack(Bitboard{1} << sq);
  }
  return table;
}();

constexpr auto kPawnAttacks = [] {
  std::array<std::array<Bitboard, kNumSquares>, kNumColors> table{};
  for (SquareIndex sq = 0; sq < kNumSquares; ++sq) {
    Bitboard bb = Bitboard{1} << sq;
    table[idx(Color::kWhite)][sq] =
        ((bb << 9U) & ~kFileA) | ((bb << 7U) & ~kFileH);
    table[idx(Color::kBlack)][sq] =
        ((bb >> 7U) & ~kFileA) | ((bb >> 9U) & ~kFileH);
  }
  return table;
}();

// ========================== Hyperbola Quintessence ==========================

// TODO Sliding pieces

// ============================= Move Generation ==============================

// TODO Need to handle promotions, castling, en passant, etc.
enum class MoveFlag : uint8_t { kQuiet };

struct Move {
 public:
  constexpr Move(SquareIndex from, SquareIndex to,
                 MoveFlag flag = MoveFlag::kQuiet)
      : data_(static_cast<uint16_t>(from | (to << 6U) |
                                    (static_cast<uint8_t>(flag) << 12U))) {}

  [[nodiscard]] constexpr SquareIndex from() const { return data_ & 0x3F; }
  [[nodiscard]] constexpr SquareIndex to() const {
    return (data_ >> 6U) & 0x3F;
  }
  [[nodiscard]] constexpr MoveFlag flag() const {
    return static_cast<MoveFlag>(data_ >> 12U);
  }

 private:
  // 16 bits: from (6), to (6), flag (4)
  uint16_t data_;
};

struct MoveList {
 public:
  void emplace_back(SquareIndex from, SquareIndex to) {
    moves_[count_++] = Move(from, to);
  }

 private:
  std::array<Move, kMaxMoves> moves_;
  size_t count_ = 0;
};

void add_moves_from_bb(SquareIndex from, Bitboard tos, MoveList& ml) {
  while (tos > 0) {
    ml.emplace_back(from, static_cast<SquareIndex>(std::countr_zero(tos)));
    tos &= tos - 1;
  }
}

constexpr Bitboard occupancy(const State& s, Color c) {
  Bitboard occ = 0;
  for (size_t p = 0; p < kNumPieces; ++p) {
    occ |= s.bbs[idx(c)][p];
  }
  return occ;
}

void generate_moves(const State& s, MoveList& ml) {
  const size_t us = idx(s.side_to_move);
  const Bitboard our_occ = occupancy(s, s.side_to_move);

  Bitboard knights = s.bbs[us][idx(Piece::kKnight)];
  while (knights > 0) {
    SquareIndex sq = static_cast<SquareIndex>(std::countr_zero(knights));
    Bitboard attacks = kKnightAttacks[sq] & ~our_occ;
    add_moves_from_bb(sq, attacks, ml);
    knights &= knights - 1;
  }

  // TODO Other pieces
}
