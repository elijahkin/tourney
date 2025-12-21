#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "../tourney_base.hpp"

// Assign human-readable names to ANSI escape codes.
constexpr std::string kCursorHome = "\x1B[H";
constexpr std::string kEraseScreen = "\x1B[2J";
constexpr std::string kForegroundBlack = "\x1B[30m";
constexpr std::string kForegroundGray = "\x1B[38;5;240m";
constexpr std::string kForegroundDefault = "\x1B[39m";
constexpr std::string kBackgroundMagenta = "\x1B[45m";
constexpr std::string kBackgroundWhite = "\x1B[47m";
constexpr std::string kBackgroundDefault = "\x1B[49m";

enum Piece : uint8_t {
  kEmpty,
  kWhiteKing,
  kWhiteQueen,
  kWhiteRook,
  kWhiteBishop,
  kWhiteKnight,
  kWhitePawn,
  kBlackKing,
  kBlackQueen,
  kBlackRook,
  kBlackBishop,
  kBlackKnight,
  kBlackPawn
};

using Square = uint8_t;

struct ChessMove {
  Square from;
  Square to;
  Piece captured;
};

class Chess final : public Game<ChessMove> {  // NOLINT
 public:
  explicit Chess(bool white_perspective)
      : white_perspective_(white_perspective) {
    // Stores the types and order of pieces in white's major rank. Black's major
    // rank is deduced from this.
    const std::vector<Piece> white_major = {
        kWhiteRook, kWhiteKnight, kWhiteBishop, kWhiteQueen,
        kWhiteKing, kWhiteBishop, kWhiteKnight, kWhiteRook};

    // Set up the board with the pieces in their starting positions. The ranks
    // are numbered starting from white's side of the board, such that white's
    // pieces start in ranks 1 and 2.
    for (char rank = '1'; rank <= '8'; ++rank) {
      for (char file = 'a'; file <= 'h'; ++file) {
        Piece piece = kEmpty;
        if (rank == '1' || rank == '8') {
          piece = white_major[file - 'a'];
        } else if (rank == '2' || rank == '7') {
          piece = kWhitePawn;
        }
        if (rank == '7' || rank == '8') {
          piece = static_cast<Piece>(piece + 6);
        }
        board_[LogicalToPhysical(file, rank)] = piece;
      }
    }
  }

  // Performs the move in memory and changes to the other player's turn.
  void MakeMove(const ChessMove &move) override {
    board_[move.to] = board_[move.from];
    board_[move.from] = kEmpty;
    white_to_move_ = !white_to_move_;
  }

  void UnmakeMove(const ChessMove &move) override {
    board_[move.from] = board_[move.to];
    board_[move.to] = move.captured;
    white_to_move_ = !white_to_move_;
  }

  // Logs the move to `history_` for printing before making it.
  void RecordMove(const ChessMove &move) {
    if (white_to_move_) {
      history_.push_back(GetAlgebraicNotation(move));
    } else {
      std::string &tmp = history_.back();
      tmp.insert(tmp.length(), " ");
      tmp.insert(tmp.length(), GetAlgebraicNotation(move));
    }
  }

  [[nodiscard]] std::vector<ChessMove> GenerateLegalMoves() const override {
    std::vector<ChessMove> moves;
    for (Square from = 0; from < 64; ++from) {
      if (IsWhite(board_[from]) == white_to_move_) {
        for (Square to : GetToSquares(from)) {
          moves.push_back({from, to, board_[to]});
        }
      }
    }
    return moves;
  }

  [[nodiscard]] std::string ToString() const override;

  // Parses user-input algebraic notation.
  [[nodiscard]] std::optional<ChessMove> Parse(
      const std::string &input) const override;

 private:
  // https://en.wikipedia.org/wiki/Chess_symbols_in_Unicode
  static constexpr std::array<std::string, 13> kUnicodePieces = {
      " ",      "\u2654", "\u2655", "\u2656", "\u2657", "\u2658", "\u2659",
      "\u265a", "\u265b", "\u265c", "\u265d", "\u265e", "\u265f"};

  // Converts the rank and file on the chess board to the index of the
  // corresponding square in `board_`.
  static Square LogicalToPhysical(char file, char rank) {
    return (8 * (rank - '1')) + (file - 'a');
  }

  static bool IsWhite(const Piece piece) {
    return (piece >= kWhiteKing) && (piece <= kWhitePawn);
  }

  [[nodiscard]] bool IsOccupied(Square square) const {
    return board_[square] != kEmpty;
  }

  [[nodiscard]] bool IsOpponent(Square square) const {
    return IsOccupied(square) && (IsWhite(board_[square]) != white_to_move_);
  }

  // https://en.wikipedia.org/wiki/Algebraic_notation_(chess)
  [[nodiscard]] std::string GetAlgebraicNotation(const ChessMove &move) const {
    static constexpr std::array<char, 6> kPieceLetters = {'K', 'Q', 'R',
                                                          'B', 'N', '\0'};
    std::string output;
    output += kPieceLetters[(board_[move.from] - 1) % 6];
    if (move.captured != kEmpty) {
      output += 'x';
    }
    output += static_cast<char>('a' + (move.to % 8));
    output += std::to_string(1 + (move.to / 8));
    return output;
  }

  void InsertToSquaresSliding(
      Square from, std::vector<Square> &tos,
      const std::vector<std::pair<int8_t, int8_t>> &directions) const;

  void InsertToSquaresLeaping(
      Square from, std::vector<Square> &tos,
      const std::vector<std::pair<int8_t, int8_t>> &directions) const;

  void InsertToSquaresPawn(Square from, std::vector<Square> &tos) const;

  // Computes a vector of squares that the piece at `from` can move to.
  [[nodiscard]] std::vector<Square> GetToSquares(Square from) const;

  // Stores the board rank-major such that the squares laid out in `board_` like
  // so: 1a ... 1h 2a ... 2h ... 7h 8a ... 8h.
  std::array<Piece, 64> board_{};

  // Records the move history in algebraic notation.
  std::vector<std::string> history_;

  // Keeps track of whose turn it is.
  bool white_to_move_ = true;

  // Determines from whose perspective we print the board.
  bool white_perspective_;
};

std::string Chess::ToString() const {
  // For each rank, prints out the rank label on the left, then the squares of
  // that rank, then every ninth move in the move history.
  std::string output = kEraseScreen + kCursorHome;
  for (size_t row = 0; row < 9; ++row) {
    const char rank =
        static_cast<char>(white_perspective_ ? '8' - row : '1' + row);
    output += (row == 8 ? ' ' : rank);
    output += ' ';
    for (size_t col = 0; col < 8; ++col) {
      const char file =
          static_cast<char>(white_perspective_ ? 'a' + col : 'h' - col);
      if (row != 8) {
        // The board is such that top left square is white for both players
        output +=
            ((row + col) % 2 == 0) ? kBackgroundWhite : kBackgroundMagenta;
        output += kForegroundBlack;
        output += kUnicodePieces[board_[LogicalToPhysical(file, rank)]];
      } else {
        output += file;
      }
      output += ' ';
    }
    output += kBackgroundDefault;
    // Prints out every ninth move in the move history offset by rank.
    output += kForegroundGray;
    output += ' ';
    for (size_t move = row; move < history_.size(); move += 9) {
      // Accommodates move numbers up to 999.
      std::string move_str = std::to_string(move + 1);
      move_str.insert(0, 3 - move_str.size(), ' ');
      move_str += ". ";
      // Pads to support algebraic notation of different lengths.
      move_str += history_[move];
      move_str.insert(move_str.end(), 14 - move_str.size(), ' ');
      output += move_str;
    }
    output += kForegroundDefault;
    output += '\n';
  }
  return output;
}

std::optional<ChessMove> Chess::Parse(const std::string &input) const {
  const std::regex san_pattern("([KQRBN]?)x?([a-h])([1-8])");

  // Ensures syntactic correctness of input.
  std::smatch san;
  if (!std::regex_match(input, san, san_pattern)) {
    return std::nullopt;
  }

  // Computes the piece type and destination from the captured groups.
  Piece type = kEmpty;
  switch (san[1].str()[0]) {
    case 'K':
      type = white_to_move_ ? kWhiteKing : kBlackKing;
      break;
    case 'Q':
      type = white_to_move_ ? kWhiteQueen : kBlackQueen;
      break;
    case 'R':
      type = white_to_move_ ? kWhiteRook : kBlackRook;
      break;
    case 'B':
      type = white_to_move_ ? kWhiteBishop : kBlackBishop;
      break;
    case 'N':
      type = white_to_move_ ? kWhiteKnight : kBlackKnight;
      break;
    default:
      type = white_to_move_ ? kWhitePawn : kBlackPawn;
      break;
  }
  const Square to = LogicalToPhysical(san[2].str()[0], san[3].str()[0]);

  // Seaches for pieces of that type that can moved to the destination.
  std::vector<Square> from_candidates;
  for (Square from = 0; from < 64; ++from) {
    if (board_[from] == type && IsWhite(board_[from]) == white_to_move_) {
      std::vector<Square> tos = GetToSquares(from);
      if (std::ranges::find(tos, to) != tos.end()) {
        from_candidates.push_back(from);
      }
    }
  }
  // Abandons the parse if there is not exactly one such piece.
  if (from_candidates.size() != 1) {
    return std::nullopt;
  }
  return ChessMove{
      .from = from_candidates[0], .to = to, .captured = board_[to]};
}

void Chess::InsertToSquaresSliding(
    Square from, std::vector<Square> &tos,
    const std::vector<std::pair<int8_t, int8_t>> &directions) const {
  const auto from_rank = static_cast<int8_t>(from / 8);
  const auto from_file = static_cast<int8_t>(from % 8);

  for (auto const &[dr, df] : directions) {
    auto to_rank = from_rank + dr;
    auto to_file = from_file + df;

    // Continue sliding until we hit the board edge
    while (to_rank >= 0 && to_rank < 8 && to_file >= 0 && to_file < 8) {
      const auto to = static_cast<Square>((to_rank * 8) + to_file);

      // We can move to a square if its empty or occupied by an opponent
      if (!IsOccupied(to) || IsOpponent(to)) {
        tos.push_back(to);
      }

      // We can't move through pieces
      if (IsOccupied(to)) {
        break;
      }

      to_rank += dr;
      to_file += df;
    }
  }
}

void Chess::InsertToSquaresLeaping(
    Square from, std::vector<Square> &tos,
    const std::vector<std::pair<int8_t, int8_t>> &directions) const {
  const auto from_rank = static_cast<int8_t>(from / 8);
  const auto from_file = static_cast<int8_t>(from % 8);

  for (auto const &[dr, df] : directions) {
    const int to_rank = from_rank + dr;
    const int to_file = from_file + df;

    // Boundary check to ensure the move stays on the board
    if (to_rank >= 0 && to_rank < 8 && to_file >= 0 && to_file < 8) {
      const auto to = static_cast<Square>((to_rank * 8) + to_file);

      // Can't move to a square occupied by your own piece
      if (!IsOccupied(to) || IsOpponent(to)) {
        tos.push_back(to);
      }
    }
  }
}

// NOLINTNEXTLINE
void Chess::InsertToSquaresPawn(Square from, std::vector<Square> &tos) const {
  const auto from_rank = static_cast<int8_t>(from / 8);
  const auto from_file = static_cast<int8_t>(from % 8);

  const int orientation = (board_[from] == kWhitePawn) ? 1 : -1;

  if ((from_rank != 7 || board_[from] != kWhitePawn) &&
      (from_rank != 0 || board_[from] != kBlackPawn)) {
    const auto forward = static_cast<Square>(from + (8 * orientation));
    if (!IsOccupied(forward)) {
      tos.push_back(forward);

      if ((from_rank == 1 && board_[from] == kWhitePawn) ||
          (from_rank == 6 && board_[from] == kBlackPawn)) {
        const auto double_forward =
            static_cast<Square>(from + (16 * orientation));
        if (!IsOccupied(double_forward)) {
          tos.push_back((double_forward));
        }
      }
    }
  }

  if ((from_file != 0 || board_[from] != kWhitePawn) &&
      (from_file != 7 || board_[from] != kBlackPawn)) {
    const auto capture_left = static_cast<Square>(from + (7 * orientation));
    if (IsOpponent(capture_left)) {
      tos.push_back(capture_left);
    }
  }
  if ((from_file != 7 || board_[from] != kWhitePawn) &&
      (from_file != 0 || board_[from] != kBlackPawn)) {
    const auto capture_right = static_cast<Square>(from + (9 * orientation));
    if (IsOpponent(capture_right)) {
      tos.push_back(capture_right);
    }
  }
}

std::vector<Square> Chess::GetToSquares(Square from) const {
  std::vector<Square> tos;

  // Define direction vectors
  const std::vector<std::pair<int8_t, int8_t>> orthogonal = {
      {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  const std::vector<std::pair<int8_t, int8_t>> diagonal = {
      {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
  const std::vector<std::pair<int8_t, int8_t>> royal_dirs = {
      {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
  const std::vector<std::pair<int8_t, int8_t>> knight_dirs = {
      {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};

  switch (board_[from]) {
    case kEmpty:
      break;
    case kWhiteKing:
    case kBlackKing:
      InsertToSquaresLeaping(from, tos, royal_dirs);
      break;
    case kWhiteQueen:
    case kBlackQueen:
      InsertToSquaresSliding(from, tos, royal_dirs);
      break;
    case kWhiteRook:
    case kBlackRook:
      InsertToSquaresSliding(from, tos, orthogonal);
      break;
    case kWhiteBishop:
    case kBlackBishop:
      InsertToSquaresSliding(from, tos, diagonal);
      break;
    case kWhiteKnight:
    case kBlackKnight:
      InsertToSquaresLeaping(from, tos, knight_dirs);
      break;
    case kWhitePawn:
    case kBlackPawn:
      InsertToSquaresPawn(from, tos);
      break;
  }
  return tos;
}
