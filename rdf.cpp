#include "Math/Vector4D.h"
#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "TCanvas.h"

#include "ROOT/TTreeProcessorMT.hxx"

#include <algorithm>
#include <iostream>

template <typename T> using Vec = const ROOT::RVec<T> &;
using ROOT::Math::XYZTVector;

// rewriting the combinations function
ROOT::RVec<ROOT::RVec<std::size_t>> combinations(std::size_t s, std::size_t n) {
  ROOT::RVec<std::size_t> indices(s);
  std::iota(indices.begin(), indices.end(), 0);

  const auto innersize = [=] {
    std::size_t inners = s - n + 1;
    for (std::size_t m = s - n + 2; m <= s; ++m)
      inners *= m;

    std::size_t factn = 1;
    for (std::size_t i = 2; i <= n; ++i)
      factn *= i;
    inners /= factn;

    return inners;
  }();

  ROOT::RVec<ROOT::RVec<std::size_t>> c(n, ROOT::RVec<std::size_t>(innersize));

  std::size_t inneridx = 0;
  for (std::size_t k = 0; k < n; k++)
    c[k][inneridx] = indices[k];
  ++inneridx;

  while (true) {
    bool run_through = true;
    long i = n - 1;
    for (; i >= 0; i--) {
      if (indices[i] != i + s - n) {
        run_through = false;
        break;
      }
    }
    if (run_through) {
      return c;
    }
    indices[i]++;
    for (long j = i + 1; j < (long)n; j++)
      indices[j] = indices[j - 1] + 1;
    for (std::size_t k = 0; k < n; k++)
      c[k][inneridx] = indices[k];
    ++inneridx;
  }
}

ROOT::RVec<std::size_t> original_find_trijet(Vec<XYZTVector> jets) {
  const auto c = ROOT::VecOps::Combinations(jets, 3);

  float distance = 1e9;
  const auto top_mass = 172.5;
  std::size_t idx = 0;
  for (auto i = 0u; i < c[0].size(); i++) {
    auto p1 = jets[c[0][i]];
    auto p2 = jets[c[1][i]];
    auto p3 = jets[c[2][i]];
    const auto tmp_mass = (p1 + p2 + p3).mass();
    const auto tmp_distance = std::abs(tmp_mass - top_mass);
    if (tmp_distance < distance) {
      distance = tmp_distance;
      idx = i;
    }
  }

  return {c[0][idx], c[1][idx], c[2][idx]};
}

ROOT::RVec<std::size_t> equivalent_find_trijet(Vec<XYZTVector> jets) {

  const auto c = combinations(jets.size(), 3);

  float distance = 1e9;
  const auto top_mass = 172.5;
  std::size_t idx = 0;
  for (auto i = 0u; i < c[0].size(); i++) {
    auto p1 = jets[c[0][i]];
    auto p2 = jets[c[1][i]];
    auto p3 = jets[c[2][i]];
    const auto tmp_mass = (p1 + p2 + p3).mass();
    const auto tmp_distance = std::abs(tmp_mass - top_mass);
    if (tmp_distance < distance) {
      distance = tmp_distance;
      idx = i;
    }
  }

  return {c[0][idx], c[1][idx], c[2][idx]};
}

// worse memory access pattern! think why!
ROOT::RVec<std::size_t> transposed_find_trijet(Vec<XYZTVector> jets) {

  const auto c = combinations(3, jets.size());

  float distance = 1e9;
  const auto top_mass = 172.5;
  std::size_t idx = 0;
  for (auto i = 0u; i < c.size(); i++) {
    auto p1 = jets[c[i][0]];
    auto p2 = jets[c[i][1]];
    auto p3 = jets[c[i][2]];
    const auto tmp_mass = (p1 + p2 + p3).mass();
    const auto tmp_distance = std::abs(tmp_mass - top_mass);
    if (tmp_distance < distance) {
      distance = tmp_distance;
      idx = i;
    }
  }

  return {c[idx][0], c[idx][1], c[idx][2]};
}

// make use of the fact that we know the size of the combinations
// no need to create extra 2d vector
ROOT::RVec<std::size_t> direct_find_trijet(Vec<XYZTVector> jets) {
  constexpr std::size_t n = 3;
  float distance = 1e9;
  const auto top_mass = 172.5;
  std::size_t idx1 = 0, idx2 = 1, idx3 = 2;

  for (std::size_t i = 0; i <= jets.size() - n; i++) {
    auto p1 = jets[i];
    for (std::size_t j = i + 1; j <= jets.size() - n + 1; j++) {
      auto p2 = jets[j];
      for (std::size_t k = j + 1; k <= jets.size() - n + 2; k++) {
        auto p3 = jets[k];
        const auto tmp_mass = (p1 + p2 + p3).mass();
        const auto tmp_distance = std::abs(tmp_mass - top_mass);
        if (tmp_distance < distance) {
          distance = tmp_distance;
          idx1 = i;
          idx2 = j;
          idx3 = k;
        }
      }
    }
  }

  return {idx1, idx2, idx3};
}

// TODO: now p1.mass() + p2.mass() != (p1 + p2).mass
// TODO: find a proxy function which is distributive
// Disclaimer: Might be impossible!
// a two-pointer approach, O(n^2) time and O(n) space
// https://leetcode.com/problems/3sum-closest/
ROOT::RVec<std::size_t> nsquare_find_trijet(Vec<XYZTVector> jets) {
  float distance = 1e9;
  const auto top_mass = 172.5;
  std::size_t idx1 = 0, idx2 = 1, idx3 = 2;

  ROOT::RVec<std::size_t> inds(jets.size());
  std::iota(inds.begin(), inds.end(), 0);
  std::sort(inds.begin(), inds.end(), [&jets](const auto &a, const auto &b) {
    return jets[a].mass() < jets[b].mass();
  });

  for (std::size_t i = 0; i <= jets.size() - 2; ++i) {
    std::size_t j = i + 1, k = jets.size() - 1;
    while (j < k) {
      const auto tmp_mass =
          (jets[inds[i]] + jets[inds[j]] + jets[inds[k]]).mass();
      if (tmp_mass == top_mass)
        return {inds[i], inds[j], inds[k]};
      const auto tmp_distance = std::abs(tmp_mass - top_mass);
      if (tmp_distance < distance) {
        distance = tmp_distance;
        idx1 = inds[i];
        idx2 = inds[j];
        idx3 = inds[k];
      }
      if (tmp_mass < top_mass)
        ++j;
      else
        --k;
    }
  }
  return {idx1, idx2, idx3};
}

float trijet_pt(Vec<float> pt, Vec<float> eta, Vec<float> phi, Vec<float> mass,
                Vec<std::size_t> idx) {
  auto p1 = ROOT::Math::PtEtaPhiMVector(pt[idx[0]], eta[idx[0]], phi[idx[0]],
                                        mass[idx[0]]);
  auto p2 = ROOT::Math::PtEtaPhiMVector(pt[idx[1]], eta[idx[1]], phi[idx[1]],
                                        mass[idx[1]]);
  auto p3 = ROOT::Math::PtEtaPhiMVector(pt[idx[2]], eta[idx[2]], phi[idx[2]],
                                        mass[idx[2]]);
  return (p1 + p2 + p3).pt();
}

void rdataframe(const std::string &method_find_trijet,
                const std::string &file_path, int nfiles) {
  std::function<ROOT::RVec<std::size_t>(const ROOT::RVec<XYZTVector> &)>
      find_trijet;

  if (method_find_trijet == "original")
    find_trijet = original_find_trijet;
  else if (method_find_trijet == "equivalent")
    find_trijet = equivalent_find_trijet;
  else if (method_find_trijet == "transposed")
    find_trijet = transposed_find_trijet;
  else if (method_find_trijet == "direct")
    find_trijet = direct_find_trijet;
  else if (method_find_trijet == "nsquare")
    find_trijet = nsquare_find_trijet;

  using ROOT::Math::PtEtaPhiMVector;
  using ROOT::VecOps::Construct;

  // it would be very interesting to look at the MT case
  // (but this would not fit in the 1 hour time limit, sorry)
  // ROOT::EnableImplicitMT();

  ROOT::RDataFrame df("Events",
                      std::vector<std::string>(
                          nfiles, file_path + "Run2012B_SingleMu_1M.root"));
  auto df2 = df.Filter([](unsigned int n) { return n >= 3; }, {"nJet"},
                       "At least three jets")
                 .Define("JetXYZT",
                         [](Vec<float> pt, Vec<float> eta, Vec<float> phi,
                            Vec<float> m) {
                           return Construct<XYZTVector>(
                               Construct<PtEtaPhiMVector>(pt, eta, phi, m));
                         },
                         {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_mass"})
                 .Define("Trijet_idx", find_trijet, {"JetXYZT"});
  auto h1 =
      df2.Define("Trijet_pt", trijet_pt,
                 {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_mass", "Trijet_idx"})
          .Histo1D<float>({"", ";Trijet pt (GeV);N_{Events}", 100, 15, 40},
                          "Trijet_pt");
  auto h2 =
      df2.Define("Trijet_leadingBtag",
                 [](const ROOT::RVec<float> &btag,
                    const ROOT::RVec<std::size_t> &idx) {
                   return Max(Take(btag, idx));
                 },
                 {"Jet_btag", "Trijet_idx"})
          .Histo1D<float>({"", ";Trijet leading b-tag;N_{Events}", 100, 0, 1},
                          "Trijet_leadingBtag");

  TCanvas c;
  c.Divide(2, 1);
  c.cd(1);
  h1->Draw();
  c.cd(2);
  h2->Draw();
  // check the canvas as a sanity check
  c.SaveAs((method_find_trijet + "_rdf.png").data());

  std::cout << "mean h1 is " << h1->GetMean() << std::endl;
  std::cout << "mean h2 is " << h2->GetMean() << std::endl;
}

int main(int argc, const char *argv[]) {
  if (argc != 4) {
    std::cout << "Usage: ./rdf <method> <filepath> <nfiles>" << std::endl;
    return 1;
  }
  std::string method_find_trijet = argv[1];
  std::string filepath = argv[2];
  int nf = atoi(argv[3]);
  std::vector<std::string> valid_methods = {"original", "equivalent",
                                            "transposed", "direct", "nsquare"};
  if (std::find(valid_methods.begin(), valid_methods.end(),
                method_find_trijet) == valid_methods.end()) {
    std::cout << "Invalid method: " << method_find_trijet << std::endl;
    std::cout << "Valid methods are: ";
    for (auto &m : valid_methods) {
      std::cout << m << " ";
    }
    std::cout << "Defaulting to original" << std::endl;
    method_find_trijet = "original";
  }
  rdataframe(method_find_trijet, filepath, nf);
}