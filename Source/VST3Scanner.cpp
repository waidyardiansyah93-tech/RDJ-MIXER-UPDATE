#include <JuceHeader.h>

namespace
{
    void addDirectory(juce::Array<juce::File>& dirs, const juce::File& dir)
    {
        if (!dir.isDirectory())
            return;
        for (const auto& existing : dirs)
            if (existing == dir)
                return;
        dirs.add(dir);
    }

    juce::Array<juce::File> getVst3Directories()
    {
        juce::Array<juce::File> dirs;
        const auto env = [](const juce::String& name) {
            return juce::SystemStats::getEnvironmentVariable(name, {});
        };

        addDirectory(dirs, juce::File(env("CommonProgramFiles")).getChildFile("VST3"));
        addDirectory(dirs, juce::File(env("CommonProgramFiles(x86)")).getChildFile("VST3"));
        addDirectory(dirs, juce::File(env("LOCALAPPDATA"))
            .getChildFile("Programs").getChildFile("Common").getChildFile("VST3"));
        addDirectory(dirs, juce::File(env("APPDATA")).getChildFile("VST3"));

        // Optional user-selected/custom VST3 folders can be added later without
        // changing the mixer process or the audio engine.
        return dirs;
    }
}

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    if (argc < 4 || juce::String(argv[1]) != "--scan")
        return 2;

    const juce::File cacheFile(argv[2]);
    const juce::File reportFile(argv[3]);
    cacheFile.getParentDirectory().createDirectory();

    juce::KnownPluginList knownPlugins;
    juce::VST3PluginFormat vst3Format;
    juce::StringArray reportLines;
    int bundlesSeen = 0;
    int pluginsFound = 0;
    int failures = 0;

    const auto dirs = getVst3Directories();
    for (const auto& root : dirs)
    {
        juce::Array<juce::File> bundles;
        root.findChildFiles(bundles, juce::File::findFilesAndDirectories, true, "*.vst3");

        for (const auto& bundle : bundles)
        {
            if (!bundle.isDirectory() && !bundle.hasFileExtension("vst3"))
                continue;

            ++bundlesSeen;
            juce::OwnedArray<juce::PluginDescription> descriptions;
            try
            {
                vst3Format.findAllTypesForFile(descriptions, bundle.getFullPathName());
                if (descriptions.isEmpty())
                {
                    reportLines.add("Tidak terbaca: " + bundle.getFileName());
                    ++failures;
                    continue;
                }

                for (auto* description : descriptions)
                {
                    if (description == nullptr)
                        continue;
                    knownPlugins.addType(*description);
                    ++pluginsFound;
                }
                reportLines.add("OK: " + bundle.getFileName()
                    + " (" + juce::String(descriptions.size()) + " format)");
            }
            catch (...)
            {
                reportLines.add("Gagal: " + bundle.getFileName());
                ++failures;
            }
        }
    }

    std::unique_ptr<juce::XmlElement> xml(knownPlugins.createXml());
    const bool saved = xml != nullptr && xml->writeTo(cacheFile);

    reportLines.insert(0, "Folder VST3: " + juce::String(dirs.size()));
    reportLines.insert(1, "Bundle diperiksa: " + juce::String(bundlesSeen));
    reportLines.insert(2, "Plugin/varian ditemukan: " + juce::String(pluginsFound));
    reportLines.insert(3, "Gagal/tidak terbaca: " + juce::String(failures));
    reportLines.add(saved ? "Cache berhasil disimpan." : "GAGAL menyimpan cache VST3.");

    reportFile.replaceWithText(reportLines.joinIntoString("\n"));
    juce::Logger::writeToLog(reportLines.joinIntoString("\n"));
    return saved ? 0 : 3;
}
