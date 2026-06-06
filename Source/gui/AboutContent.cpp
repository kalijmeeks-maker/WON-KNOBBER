/*
    AboutContent.cpp — see header. Verbatim About-card credit block + full third-party
    licence body, single-sourced for the front + rear About/Licences modals.
*/
#include "AboutContent.h"

namespace wk::about
{
juce::String creditBlockText()
{
    // Credit block — verbatim per Design §3 (legally precise; hardcoded, NOT paraphrased).
    // Two-license picture: the plugin is PolyForm-NC, the Airwindows core is MIT — both appear.
    const juce::String copy = juce::String(juce::CharPointer_UTF8("\xc2\xa9"));     // ©
    const juce::String dash = juce::String(juce::CharPointer_UTF8("\xe2\x80\x94")); // —
    const juce::String mid = juce::String(juce::CharPointer_UTF8("\xc2\xb7"));      // ·
    const juce::String tm = juce::String(juce::CharPointer_UTF8("\xe2\x84\xa2"));   // ™

    juce::String credit;
    credit << "Saturation core derived from Airwindows " << dash << " " << copy
           << " 2018 Chris Johnson, used under the MIT licence.\n"
           << "Cabinet impulse responses under their respective licences (see notices). Neural models " << copy
           << " Kali Meeks.\n"
           << "WON KNOBBER " << copy << " 2026 Kali Meeks " << mid
           << " PolyForm Noncommercial 1.0.0. Built with JUCE 8. VST3" << tm << " Steinberg Media Technologies.";
    return credit;
}

juce::String licencesBodyText()
{
    // Hardcoded static text (matches how the About card builds its credit block). The MIT licence
    // and the RTNeural BSD-3-Clause licence below are copied VERBATIM from THIRD_PARTY_LICENSES.md;
    // do not paraphrase. The trailing note covers per-IR / per-model notices, which are appended as
    // those assets ship.
    juce::String t;
    t << "Airwindows (saturation algorithms)\n"
      << "\n"
      << "The saturation transfer functions in Source/dsp/AirwindowsShapers.h\n"
      << "(Density3, Mojo, Spiral2 presence, PurestSaturation) are derived from Airwindows.\n"
      << "\n"
      << "Source: https://github.com/airwindows/airwindows\n"
      << "Copyright (c) 2018 Chris Johnson\n"
      << "License: MIT\n"
      << "\n"
      << "MIT License\n"
      << "\n"
      << "Copyright (c) 2018 Chris Johnson\n"
      << "\n"
      << "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
      << "of this software and associated documentation files (the \"Software\"), to deal\n"
      << "in the Software without restriction, including without limitation the rights\n"
      << "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
      << "copies of the Software, and to permit persons to whom the Software is\n"
      << "furnished to do so, subject to the following conditions:\n"
      << "\n"
      << "The above copyright notice and this permission notice shall be included in all\n"
      << "copies or substantial portions of the Software.\n"
      << "\n"
      << "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
      << "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
      << "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
      << "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
      << "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
      << "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
      << "SOFTWARE.\n"
      << "\n"
      << "\n"
      << "RTNeural (neural inference engine)\n"
      << "\n"
      << "The neural character models (TAPE-1971, VALVE-CLASS A, TRANSISTOR-FET, IRON-TRANSFORMER)\n"
      << "run via RTNeural inference in Source/dsp/NeuralModel.*.\n"
      << "\n"
      << "Source: https://github.com/jatinchowdhury18/RTNeural\n"
      << "Copyright (c) 2020, jatinchowdhury18\n"
      << "License: BSD 3-Clause\n"
      << "\n"
      << "BSD 3-Clause License\n"
      << "\n"
      << "Copyright (c) 2020, jatinchowdhury18\n"
      << "All rights reserved.\n"
      << "\n"
      << "Redistribution and use in source and binary forms, with or without\n"
      << "modification, are permitted provided that the following conditions are met:\n"
      << "\n"
      << "1. Redistributions of source code must retain the above copyright notice, this\n"
      << "   list of conditions and the following disclaimer.\n"
      << "\n"
      << "2. Redistributions in binary form must reproduce the above copyright notice,\n"
      << "   this list of conditions and the following disclaimer in the documentation\n"
      << "   and/or other materials provided with the distribution.\n"
      << "\n"
      << "3. Neither the name of the copyright holder nor the names of its\n"
      << "   contributors may be used to endorse or promote products derived from\n"
      << "   this software without specific prior written permission.\n"
      << "\n"
      << "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
      << "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
      << "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
      << "DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n"
      << "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
      << "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n"
      << "SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"
      << "CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
      << "OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
      << "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
      << "\n"
      << "\n"
      << "Framework (JUCE 8 / VST3)\n"
      << "\n"
      << "Built with the JUCE framework. VST3 is a trademark of Steinberg Media Technologies\n"
      << "GmbH. WON KNOBBER (c) 2026 Kali Meeks, distributed under the PolyForm Noncommercial\n"
      << "Licence 1.0.0.\n"
      << "\n"
      << "Per-IR and per-model notices are added here as those assets ship.";
    return t;
}

juce::String versionString()
{
    // Build version from the JUCE-generated macro (CMake project VERSION). Single-sourced so it
    // auto-bumps at ship; never hardcode the version elsewhere.
   #ifdef JucePlugin_VersionString
    return "v" JucePlugin_VersionString;
   #else
    return "v0.1.0";
   #endif
}
} // namespace wk::about
