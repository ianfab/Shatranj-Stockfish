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

#include <fstream>
#include <iostream>
#include <istream>
#include <vector>

#include "position.h"

using namespace std;

namespace {

const vector<string> Defaults = {
  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w 0 1",
  "8/8/2k1nb2/3q3P/2p5/1NR1K3/8/8 w 0 1",
  "r2kq2r/p1nppp2/1ppbbnpp/8/2P5/1PNBBPPN/P2PPQ1P/R2K3R b 1 10",
  "5r1r/pppq2k1/2nbbnpp/4p3/4P3/2PBQNPP/PP1NP3/R2K3R b 1 21",
  "3r4/1pk2q1p/2p1b3/2n2p2/1bPN3P/1P1PQ1P1/r3P3/1R1K2R1 b 0 32",
  "r2k4/2n3rp/p1pn1p2/PpNpP3/2b1q2R/1PP1B1P1/2KQ1PN1/R4B2 b 0 33",
  "2r5/R4nk1/p1p2n1p/P3p3/2p1p2R/2P1B1P1/2KQ1P2/5B2 w 0 46",
  "6b1/7P/2k5/8/8/4K3/8/8 w 0 1", // bare king in 1
  "4k3/4P3/4K3/p7/P7/8/8/8 b 0 1", // stalemate
  "6k1/6Rp/1p3PNn/1P2B3/4p3/2q5/2b2K2/3r4 b 0 1" // mate
};

} // namespace

/// setup_bench() builds a list of UCI commands to be run by bench. There
/// are five parameters: TT size in MB, number of search threads that
/// should be used, the limit value spent for each position, a file name
/// where to look for positions in FEN format and the type of the limit:
/// depth, perft, nodes and movetime (in millisecs).
///
/// bench -> search default positions up to depth 13
/// bench 64 1 15 -> search default positions up to depth 15 (TT = 64MB)
/// bench 64 4 5000 current movetime -> search current position with 4 threads for 5 sec
/// bench 64 1 100000 default nodes -> search default positions for 100K nodes each
/// bench 16 1 5 default perft -> run a perft 5 on default positions

vector<string> setup_bench(const Position& current, istream& is) {

  vector<string> fens, list;
  string go, token;

  // Assign default values to missing arguments
  string ttSize    = (is >> token) ? token : "16";
  string threads   = (is >> token) ? token : "1";
  string limit     = (is >> token) ? token : "13";
  string fenFile   = (is >> token) ? token : "default";
  string limitType = (is >> token) ? token : "depth";

  go = "go " + limitType + " " + limit;

  if (fenFile == "default")
      fens = Defaults;

  else if (fenFile == "current")
      fens.push_back(current.fen());

  else
  {
      string fen;
      ifstream file(fenFile);

      if (!file.is_open())
      {
          cerr << "Unable to open file " << fenFile << endl;
          exit(EXIT_FAILURE);
      }

      while (getline(file, fen))
          if (!fen.empty())
              fens.push_back(fen);

      file.close();
  }

  list.emplace_back("ucinewgame");
  list.emplace_back("setoption name Threads value " + threads);
  list.emplace_back("setoption name Hash value " + ttSize);

  for (const string& fen : fens)
      if (fen.find("setoption") != string::npos)
          list.emplace_back(fen);
      else
      {
          list.emplace_back("position fen " + fen);
          list.emplace_back(go);
      }

  return list;
}
