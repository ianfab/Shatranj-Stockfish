/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <cassert>

#include "bitboard.h"
#include "endgame.h"
#include "movegen.h"

using std::string;

namespace {

  // Table used to drive the king towards the edge of the board
  // in KX vs K and KQ vs KR endgames.
  const int PushToEdges[SQUARE_NB] = {
    100, 90, 80, 70, 70, 80, 90, 100,
     90, 70, 60, 50, 50, 60, 70,  90,
     80, 60, 40, 30, 30, 40, 60,  80,
     70, 50, 30, 20, 20, 30, 50,  70,
     70, 50, 30, 20, 20, 30, 50,  70,
     80, 60, 40, 30, 30, 40, 60,  80,
     90, 70, 60, 50, 50, 60, 70,  90,
    100, 90, 80, 70, 70, 80, 90, 100
  };

  // Tables used to drive a piece towards or away from another piece
  const int PushClose[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };
  const int PushAway [8] = { 0, 5, 20, 40, 60, 80, 90, 100 };

  // Pawn Rank based scaling factors used in KRPPKRP endgame
  const int KRPPKRPScaleFactors[RANK_NB] = { 0, 9, 10, 14, 21, 44, 0, 0 };

#ifndef NDEBUG
  bool verify_material(const Position& pos, Color c, Value npm, int pawnsCnt) {
    return pos.non_pawn_material(c) == npm && pos.count<PAWN>(c) == pawnsCnt;
  }
#endif

  // Map the square as if strongSide is white and strongSide's only pawn
  // is on the left half of the board.
  Square normalize(const Position& pos, Color strongSide, Square sq) {

    assert(pos.count<PAWN>(strongSide) == 1);

    if (file_of(pos.square<PAWN>(strongSide)) >= FILE_E)
        sq = Square(sq ^ 7); // Mirror SQ_H1 -> SQ_A1

    if (strongSide == BLACK)
        sq = ~sq;

    return sq;
  }

} // namespace


/// Endgames members definitions

Endgames::Endgames() {

  add<KRKP>("KRKP");
  add<KRKB>("KRKB");
  add<KRKN>("KRKN");
  add<KNKB>("KNKB");
  add<KQKP>("KQKP");
  add<KRKQ>("KRKQ");
  add<KPKP>("KPKP");
  add<KQQKQ>("KQQKQ");

  add<KRPKR>("KRPKR");
  add<KRPPKRP>("KRPPKRP");
}

/// KR vs KP.
template<>
Value Endgame<KRKP>::operator()(const Position& pos) const {

  assert(verify_material(pos, strongSide, RookValueMg, 0));
  assert(verify_material(pos, weakSide, VALUE_ZERO, 1));

  Square wksq = relative_square(strongSide, pos.square<KING>(strongSide));
  Square psq  = relative_square(strongSide, pos.square<PAWN>(weakSide));

  Value result = RookValueEg - distance(wksq, psq);

  return strongSide == pos.side_to_move() ? result : -result;
}


/// KR vs KB.
template<>
Value Endgame<KRKB>::operator()(const Position& pos) const {

  assert(verify_material(pos, strongSide, RookValueMg, 0));
  assert(verify_material(pos, weakSide, BishopValueMg, 0));

  Square winnerKSq = pos.square<KING>(strongSide);
  Square loserKSq = pos.square<KING>(weakSide);

  Value result =  RookValueEg
                - BishopValueEg
                + PushToEdges[loserKSq]
                + PushClose[distance(winnerKSq, loserKSq)];

  return strongSide == pos.side_to_move() ? result : -result;
}


/// KR vs KN. The attacking side has some winning chances,
/// particularly if the king and the knight are far apart.
template<>
Value Endgame<KRKN>::operator()(const Position& pos) const {

  assert(verify_material(pos, strongSide, RookValueMg, 0));
  assert(verify_material(pos, weakSide, KnightValueMg, 0));

  Square bksq = pos.square<KING>(weakSide);
  Square bnsq = pos.square<KNIGHT>(weakSide);
  Value result = Value(PushToEdges[bksq] + PushAway[distance(bksq, bnsq)]);
  return strongSide == pos.side_to_move() ? result : -result;
}


/// KN vs KB. Drawish.
template<>
Value Endgame<KNKB>::operator()(const Position& pos) const {

  assert(verify_material(pos, strongSide, KnightValueMg, 0));
  assert(verify_material(pos, weakSide, BishopValueMg, 0));

  Square wksq = pos.square<KING>(strongSide);
  Square wnsq = pos.square<KNIGHT>(strongSide);
  Square bksq = pos.square<KING>(weakSide);
  Square bbsq = pos.square<BISHOP>(weakSide);

  Value result =  Value(PushToEdges[bbsq])
                + PushClose[distance(wksq, bbsq)]
                + PushClose[distance(wnsq, bbsq)]
                + PushAway[distance(bksq, bbsq)];

  return strongSide == pos.side_to_move() ? result : -result;
}


/// KQ vs KP.
template<>
Value Endgame<KQKP>::operator()(const Position& pos) const {

  assert(verify_material(pos, strongSide, QueenValueMg, 0));
  assert(verify_material(pos, weakSide, VALUE_ZERO, 1));

  Square winnerKSq = pos.square<KING>(strongSide);
  Square queenSq = pos.square<QUEEN>(strongSide);
  Square pawnSq = pos.square<PAWN>(weakSide);

  Value result =  QueenValueEg
                + PushClose[distance(winnerKSq, pawnSq)]
                + PushClose[distance(winnerKSq, queenSq)];

  return strongSide == pos.side_to_move() ? result : -result;
}


/// KR vs KQ.
template<>
Value Endgame<KRKQ>::operator()(const Position& pos) const {

  assert(verify_material(pos, strongSide, RookValueMg, 0));
  assert(verify_material(pos, weakSide, QueenValueMg, 0));

  Square winnerKSq = pos.square<KING>(strongSide);
  Square loserKSq = pos.square<KING>(weakSide);

  Value result =  RookValueEg
                - QueenValueEg
                + PushToEdges[loserKSq]
                + PushClose[distance(winnerKSq, loserKSq)];

  return strongSide == pos.side_to_move() ? result : -result;
}

/// KQQ vs KQ.
template<>
Value Endgame<KQQKQ>::operator()(const Position& pos) const {

  assert(verify_material(pos, strongSide, 2 * QueenValueMg, 0));
  assert(verify_material(pos, weakSide, QueenValueMg, 0));

  Square wksq = pos.square<KING>(strongSide);
  Square bksq = pos.square<KING>(weakSide);
  Square bqsq = pos.square<QUEEN>(weakSide);

  Value result =  Value(PushToEdges[bqsq])
                + PushToEdges[bksq]
                + PushClose[distance(wksq, bqsq)]
                + PushAway[distance(bksq, bqsq)];

  if (!(pos.pieces(QUEEN) & DarkSquares) || !(pos.pieces(QUEEN) & ~DarkSquares))
      result += VALUE_KNOWN_WIN;

  return strongSide == pos.side_to_move() ? result : -result;
}

/// KP vs KP.
template<>
Value Endgame<KPKP>::operator()(const Position& pos) const {

  assert(verify_material(pos, strongSide, VALUE_ZERO, 1));
  assert(verify_material(pos, weakSide,   VALUE_ZERO, 1));

  // Assume strongSide is white and the pawn is on files A-D
  Square wksq = normalize(pos, strongSide, pos.square<KING>(strongSide));
  Square bksq = normalize(pos, strongSide, pos.square<KING>(weakSide));
  Square wpsq = normalize(pos, strongSide, pos.square<PAWN>(strongSide));
  Square bpsq = normalize(pos, strongSide, pos.square<PAWN>(weakSide));

  Value result =  Value(PushClose[distance(wksq, bpsq)])
                - Value(PushClose[distance(bksq, wpsq)]);

  return strongSide == pos.side_to_move() ? result : -result;
}


/// KRP vs KR. Drawish.
template<>
ScaleFactor Endgame<KRPKR>::operator()(const Position&) const { return SCALE_FACTOR_DRAW; }


/// KRPP vs KRP. There is just a single rule: if the stronger side has no passed
/// pawns and the defending king is actively placed, the position is drawish.
template<>
ScaleFactor Endgame<KRPPKRP>::operator()(const Position& pos) const {

  assert(verify_material(pos, strongSide, RookValueMg, 2));
  assert(verify_material(pos, weakSide,   RookValueMg, 1));

  Square wpsq1 = pos.squares<PAWN>(strongSide)[0];
  Square wpsq2 = pos.squares<PAWN>(strongSide)[1];
  Square bksq = pos.square<KING>(weakSide);

  // Does the stronger side have a passed pawn?
  if (pos.pawn_passed(strongSide, wpsq1) || pos.pawn_passed(strongSide, wpsq2))
      return SCALE_FACTOR_NONE;

  Rank r = std::max(relative_rank(strongSide, wpsq1), relative_rank(strongSide, wpsq2));

  if (   distance<File>(bksq, wpsq1) <= 1
      && distance<File>(bksq, wpsq2) <= 1
      && relative_rank(strongSide, bksq) > r)
  {
      assert(r > RANK_1 && r < RANK_7);
      return ScaleFactor(KRPPKRPScaleFactors[r]);
  }
  return SCALE_FACTOR_NONE;
}

