#include <JuceHeader.h>
#include <iostream>
#include <memory>

namespace
{
    juce::File getCacheFile()
    {
        const auto appData = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory);

        return appData.getChildFile("RDJ_MIXER")
                      .getChildFile("VST3PluginList.xml");
    }

    void print(const juce::String& message)
    {
        std::cout << message << std::endl;
    }

    void addSearchPathIfPresent(juce::FileSearchPath& paths,
                                const juce::File& folder)
    {
        if (!folder.isDirectory())
            return;

        const auto candidate = folder.getFullPathName();

        for (int i = 0; i < paths.getNumPaths(); ++i)
        {
            if (paths[i].getFullPathName().equalsIgnoreCase(candidate))
                return;
        }

        paths.add(folder);
    }

    bool savePluginCache(const juce::KnownPluginList& knownPlugins,
                         const juce::File& cacheFile)
    {
        std::unique_ptr<juce::XmlElement> xml(knownPlugins.createXml());

        if (xml == nullptr)
            return false;

        // Save after every completed scan step. If a later third-party plugin
        // crashes the scanner process, plugins found earlier remain available.
        return xml->writeTo(cacheFile);
    }

    void loadExistingCache(juce::KnownPluginList& knownPlugins,
                           const juce::File& cacheFile)
    {
        if (!cacheFile.existsAsFile())
            return;

        juce::XmlDocument document(cacheFile);
        std::unique_ptr<juce::XmlElement> xml(document.getDocumentElement());

        if (xml != nullptr)
        {
            knownPlugins.recreateFromXml(*xml);
            print("Cache sebelumnya dimuat: "
                  + juce::String(knownPlugins.getNumTypes()) + " plugin.");
        }
        else
        {
            print("PERINGATAN: Cache lama tidak valid. Scan dimulai dari daftar kosong.");
        }
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    print("======================================");
    print("       RDJ MIXER - VST3 SCANNER");
    print("======================================");

#if ! JUCE_PLUGINHOST_VST3
    print("ERROR: JUCE_PLUGINHOST_VST3 tidak aktif untuk target RDJ_VST3_Scanner.");
    print("Pastikan CMakeLists.txt menetapkan JUCE_PLUGINHOST_VST3=1.");
    return 1;
#else
    juce::AudioPluginFormatManager formatManager;
    formatManager.addFormat(std::make_unique<juce::VST3PluginFormat>());

    juce::AudioPluginFormat* vst3Format = nullptr;

    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format != nullptr && format->getName().equalsIgnoreCase("VST3"))
        {
            vst3Format = format;
            break;
        }
    }

    if (vst3Format == nullptr)
    {
        print("ERROR: Format VST3 tidak berhasil didaftarkan.");
        return 2;
    }

    const auto cacheFile = getCacheFile();
    const auto cacheDirectory = cacheFile.getParentDirectory();

    if (!cacheDirectory.createDirectory())
    {
        print("ERROR: Gagal membuat folder cache:");
        print(cacheDirectory.getFullPathName());
        return 3;
    }

    juce::FileSearchPath searchPaths = vst3Format->getDefaultLocationsToSearch();

#if JUCE_WINDOWS
    // These are the standard system-wide VST3 locations on 64-bit Windows.
    // PluginDirectoryScanner searches inside these roots, including vendor
    // subfolders such as FabFilter, Antares, Arturia, Steinberg, etc.
    addSearchPathIfPresent(searchPaths,
        juce::File("C:\\Program Files\\Common Files\\VST3"));

    addSearchPathIfPresent(searchPaths,
        juce::File("C:\\Program Files (x86)\\Common Files\\VST3"));
#endif

    print("Lokasi pencarian VST3:");
    print(searchPaths.toString());

    juce::KnownPluginList knownPlugins;
    loadExistingCache(knownPlugins, cacheFile);

    const auto deadMansPedal = cacheDirectory.getChildFile("VST3Scanner_deadman.txt");

    // IMPORTANT:
    // Do not delete the dead-man's-pedal file here. JUCE uses it to identify
    // a plugin that crashed during the previous scan. Removing it would make
    // the scanner retry the same faulty plugin on every launch.
    if (deadMansPedal.existsAsFile())
    {
        print("Penanda scan sebelumnya ditemukan:");
        print(deadMansPedal.getFullPathName());
        print("JUCE akan menggunakan penanda ini untuk menghindari pengulangan "
              "plugin yang menyebabkan scan terhenti.");
    }

    juce::PluginDirectoryScanner scanner(
        knownPlugins, *vst3Format, searchPaths, true, deadMansPedal);

    juce::String pluginBeingScanned;
    int processedFiles = 0;
    int cacheSaveFailures = 0;

    print("--------------------------------------");
    print("Memulai scan VST3.");
    print("Jangan tutup jendela scanner sampai selesai.");
    print("Hasil yang sudah ditemukan akan disimpan bertahap.");

    while (scanner.scanNextFile(true, pluginBeingScanned))
    {
        ++processedFiles;
        print("[" + juce::String(processedFiles) + "] Memeriksa: "
              + pluginBeingScanned);

        if (!savePluginCache(knownPlugins, cacheFile))
        {
            ++cacheSaveFailures;
            print("PERINGATAN: Gagal menyimpan cache sementara.");
        }
    }

    if (!savePluginCache(knownPlugins, cacheFile))
    {
        print("ERROR: Gagal menyimpan cache VST3 ke:");
        print(cacheFile.getFullPathName());
        return 4;
    }

    print("--------------------------------------");
    print("SCAN SELESAI");
    print("Jumlah plugin dalam daftar: "
          + juce::String(knownPlugins.getNumTypes()));
    print("File yang diproses pada sesi ini: "
          + juce::String(processedFiles));
    print("Cache tersimpan di:");
    print(cacheFile.getFullPathName());

    if (cacheSaveFailures > 0)
    {
        print("PERINGATAN: Ada " + juce::String(cacheSaveFailures)
              + " kegagalan penyimpanan cache sementara. Cache akhir berhasil disimpan.");
    }

    if (deadMansPedal.existsAsFile())
    {
        print("PERHATIAN: Penanda plugin bermasalah masih ada:");
        print(deadMansPedal.getFullPathName());
        print("Periksa file tersebut jika ada plugin yang belum berhasil dipindai.");
    }
    else
    {
        print("Tidak ada penanda crash yang tertinggal.");
    }

    print("Anda dapat menutup scanner dan membuka RDJ MIXER.");
    return 0;
#endif
}
