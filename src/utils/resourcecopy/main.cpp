//=== CResourceCopy -> Written by Unusuario2, https://github.com/Unusuario2  ====//
//
// Purpose:
//
// $NoKeywords: $
//===============================================================================//
#include <windows.h>
#include <vector>
#include <array>
#include <tier1/strtools.h>
#include <tier0/icommandline.h>
#include <tools_minidump.h>
#include <loadcmdline.h>
#include <cmdlib.h>
#include <filesystem_init.h>
#include <filesystem_tools.h>
#include <colorschemetools.h>
#include "resourcecopy/cresourcecopy.hpp"

#pragma warning(disable : 4238)

// TODO: Make the launcher copy a single file, given two paths, and given a wildcard!
// (e.g) -> C:\\hl2\\hl2_addons\\hl2_test.txt C:\\hl2\\hl2_copy
// (e.g) -> C:\\hl2\\hl2_addons\\hl2_test\\   C:\\hl2\\hl2_copy  THIS IS SHALLOW!!
// (e.g) -> C:\\hl2\\hl2_addons\\hl2_test\\*  C:\\hl2\\hl2_copy  RECURSIVE!


//-----------------------------------------------------------------------------
// Purpose: Global vars
//-----------------------------------------------------------------------------
bool			g_bQuiet            = false;
bool            g_bCreateLogFile    = false;
bool            g_bDeleteDir        = false;
bool            g_bPrintDirContents = false;
bool            g_bTransferFiles    = false;
bool            g_bOverWriteFiles   = true;
bool            g_bSingleFile       = false;
bool            g_bProcessShallow   = false;
int             g_iThreads          = -1;
char            g_szLogFile[MAX_PATH];
char            g_szDeleteDirPath[MAX_PATH];
char            g_szSourceDir[MAX_PATH];
char            g_szDestinationDir[MAX_PATH];
CResourceCopy*  g_pResourceCopy;
FileList        g_vGlobalCopyList;



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void PrintHeader()
{
    ColorSpewMessage(SPEW_MESSAGE, &ColorHeader, "\nUnusuario2 - resourcecopy.exe (Build: %s %s)\n", __DATE__, __TIME__);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void PrintUsage(int argc, char* argv[])
{
    Msg("Usage: resourcecopy.exe [options] <source> <destination>\n\n");
    ColorSpewMessage(SPEW_MESSAGE, &ColorHeader, " General Options:\n");
    Msg("   <source>:                       Source filename, folder or folder + wildcard (e.g., \"hl2_addons\\hl2_test_map\\*\").\n"
        "   <destination>:                  Destination filename or folder (e.g., \"hl2_addons\\hl2_test_map_copy\").\n"
        "\n");

    ColorSpewMessage(SPEW_MESSAGE, &ColorHeader, " File Control Options:\n");
    Msg("   -printdircontent or -pdc <path> Print in the console the files found at the source dir. (Note: Wildcards are acepted).\n"
        "   -shallow:                       Don't recurse for wildcards.\n"
        "   -deletedir <path>:              Given a dir deletes all the files inside. (Note: Wildcards are acepted).\n"
        "   -safe:                          Don't overwrite existing target files.\n"
        //"   -skip <substring>:              Skip files whose full path contains this substring. Can be repeated.\n" // TODO!
        //"   -rename <from_tag> <to_tag>:    Rename files where 'from_tag' occurs in the filename.\n"
        "   -transferfiles:                 Copies the files found at the source dir to the destination dir and deletes the source dir.\n"
        "\n");

    ColorSpewMessage(SPEW_MESSAGE, &ColorHeader, " Logging Options:\n");
    Msg("   -fulllog <filename>:            Write a full operation log to the specified file.\n"
        "\n");

    ColorSpewMessage(SPEW_MESSAGE, &ColorHeader, "Other Options:\n");
    Msg("   -help or -?:                    Print help.\n"
        "   -v or verbose:                  Enables verbose.\n"
        "   -quiet                          Prints minimal text.\n"
        "   -FullMinidumps:                 In case of a crash, writes large .mdmp files.\n"
        "   -threads <int>:                 Set the number of threads for the program to use.\n" 
        "\n");

    ColorSpewMessage(SPEW_MESSAGE, &ColorHeader, " Example Usage:\n");
    Msg("   D:\\dev\\game>resourcecopy.exe  -fulllog log.txt C:\\hl2\\hl2_addons\\hl2_test\\* C:\\hl2\\hl2_addons\\hl2_copy\n"
        "\n");

    DeleteCmdLine(argc, argv);
    CmdLib_Cleanup();
    CmdLib_Exit(-1);
}


//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
static void ParseCommandline(int argc, char* argv[])
{
    if (argc <= 2)
    {
        PrintUsage(argc, argv);
    }

    for (int i = 1; i < argc; ++i)
    {
        V_FixSlashes(argv[i]);
        if (!V_stricmp(argv[i], "-?") || !V_stricmp(argv[i], "-help") || argc == 1)
        {
            PrintUsage(argc, argv);
        }
        else if (!V_stricmp(argv[i], "-v") || !V_stricmp(argv[i], "-verbose"))
        {
            verbose = true;
            g_bQuiet = false;
        }
        else if (!V_stricmp(argv[i], "-quiet"))
        {
            verbose = false;
            g_bQuiet = true;
        }
        else if (!V_stricmp(argv[i], "-safe"))
        {
            g_bOverWriteFiles = false;
        }
        else if (!V_stricmp(argv[i], "-shallow"))
        {
            g_bProcessShallow = true;
        }
        else if (!V_stricmp(argv[i], "-threads"))
        {
            if (++i < argc && argv[i] != '\0')
            {
                int iTemp = V_atoi(argv[i]);
                if(iTemp < 1)
                {
                    Warning("Expected value greater or equal that 1!\n");
                    PrintUsage(argc, argv);
                }
            }
            else
            {
                Warning("Expected value after '-threads'\n");
                PrintUsage(argc, argv);
            }
        }
        else if (!V_stricmp(argv[i], "-transferfiles"))
        {
            g_bTransferFiles = true;
        }
        else if (!V_stricmp(argv[i], "-printdircontent") || !V_stricmp(argv[i], "-pdc"))
        {
            if (++i < argc && *argv[i])
            {
                g_bPrintDirContents = true;
                V_strcpy(g_szSourceDir, argv[i]);
            }
            else
            {
                Warning("Expected a path after '-printdircontent'!\n");
                PrintUsage(argc, argv);
            }
        }
        else if (!V_stricmp(argv[i], "-FullMinidumps"))
        {
            EnableFullMinidumps(true);
        }
        else if (!V_stricmp(argv[i], "-fulllog"))
        {
            if (++i < argc && *argv[i])
            {
                g_bCreateLogFile = true;
                V_strcpy(g_szLogFile, argv[i]);
            }
            else
            {
                Error("Error: \'-fulllog\' requires a valid path argument.\n");
            }
        }
        else if (!V_stricmp(argv[i], "-deletedir"))
        {
            if (++i < argc && *argv[i])
            {
                g_bDeleteDir = true;
                V_strcpy(g_szDeleteDirPath, argv[i]);
            }
            else
            {
                Warning("Error: \'-deletedir\' requires a valid path argument.\n");
                PrintUsage(argc, argv);
            }
        }
        else
        {
            if (!(i == argc - 2 || i == argc - 1))
            {
                Warning("\nUnknown option \'%s\'\n", argv[i]);
                PrintUsage(argc, argv);
            }
        }
    }

    if (verbose)
    {
        ColorSpewMessage(SPEW_MESSAGE, &ColorHeader, "Command line:\n\t");
        for (int i = 1; i < argc; i++)
        {
            Msg("%s ", argv[i]);
        }
        Msg("\n\n");
    }

    if (g_bDeleteDir)
        return;

    if (g_bPrintDirContents)
        return;


    if ((argv[argc - 2] && (argv[argc - 2][0] != '\0')) && (argv[argc - 1] && (argv[argc - 1][0] != '\0')))
    {
        V_strcpy(g_szSourceDir, argv[argc - 2]);
        V_strcpy(g_szDestinationDir, argv[argc-1]);
    }
    else
    {
        Error("Source or destination path are NULL!\n"
              "Check the if the input are paths (note these need to be ALWAYS last in the commnad line!\n"
              "Source: %s\n"
              "Destination: %s\n", 
              argv[argc - 2], argv[argc - 1]);
    }

    char* ptr = V_strrchr(g_szSourceDir, '.');
    if (ptr && *(ptr + 1) != '\0' && *(ptr - 1) != '*')
    {
        g_bSingleFile = true;
    }

    if(g_bProcessShallow && !g_bSingleFile)
    {
        char* ptr = V_strrchr(g_szSourceDir, '*');
        if (ptr)
            *(ptr - 1) = '\0';
        else
            Warning("The path: %s is not a wildcard!\n"
                    "This will not change the behavior of the program, as normal paths are always treated as shallow.\n",
                    g_szSourceDir);
    }

    // Remove the final trailing slash of the paths
    if (!g_bSingleFile)
    {
        if (g_szSourceDir[V_strlen(g_szSourceDir) - 2] == '\\')
        {
            g_szSourceDir[V_strlen(g_szSourceDir) - 2] = '\0';
        }
    }

    if (g_szDestinationDir[V_strlen(g_szDestinationDir) - 2] == '\\')
    {
        g_szDestinationDir[V_strlen(g_szDestinationDir) - 2] = '\0';
    }
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void LoadSystemModules()
{
    float start = Plat_FloatTime();
    qprintf("Loading resourcecopy.exe modules... ");

    CSysModule* pFileSystem_stdio = Sys_LoadModule("filesystem_stdio.dll");
    if (!pFileSystem_stdio)
        Error("\nFailed to open filesystem_stdio.dll module.\n");

    CreateInterfaceFn factory = Sys_GetFactory(pFileSystem_stdio);
    if (factory)
    {
        g_pFileSystem = (IBaseFileSystem*)factory(BASEFILESYSTEM_INTERFACE_VERSION, nullptr);
        g_pFullFileSystem = (IFileSystem*)factory(FILESYSTEM_INTERFACE_VERSION, nullptr);
    }
    else
    {
        Error("\nFailed to load filesystem_stdio.dll!\n");
    }

    if (!g_pFileSystem || !g_pFullFileSystem)
        Error("\nFailed to create interface from filesystem_stdio.dll!\n");

    qprintf("done(%.2f)\n", Plat_FloatTime() - start);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void Init(int argc, char* argv[])
{
    SetupDefaultToolsMinidumpHandler();
    CommandLine()->CreateCmdLine(argc, argv);
    InstallSpewFunction();
    PrintHeader();
    ParseCommandline(argc, argv);
    LoadSystemModules();

    if (g_bCreateLogFile)
        SetSpewFunctionLogFile(g_szLogFile);

    g_pResourceCopy = new CResourceCopy();
    if (g_iThreads != -1)
        g_pResourceCopy->SetThreads(g_iThreads);

    if (verbose)
        g_pResourceCopy->SetVerboseSpewMode();
    else if (g_bQuiet)
        g_pResourceCopy->SetQuietSpewMode();
    else
        g_pResourceCopy->SetNormalSpewMode();

}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void Destroy(int argc, char* argv[], const int iExitCode = 0)
{
    g_pResourceCopy->GenerateGlobalOperationReport();
    delete g_pResourceCopy;

    DeleteCmdLine(argc, argv);
    CmdLib_Cleanup();
    CmdLib_Exit(iExitCode);
}


//-----------------------------------------------------------------------------
// Purpose:   Main funtion
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    Init(argc, argv);

    // Sanity checks...
    if (g_bPrintDirContents && g_bSingleFile)
    {
        Warning("Cannot print contents while copying a file. Skipping.\n");
        g_bPrintDirContents = false;
    }
    if ((g_bDeleteDir && g_bSingleFile) || (g_bDeleteDir && g_bPrintDirContents) ||
        (g_bDeleteDir && g_bTransferFiles) || (g_bPrintDirContents && g_bTransferFiles))
    {
        ColorSpewMessage(SPEW_MESSAGE, &ColorUnSucesfull,
            "Invalid operation combination detected!\n"
            "Run resourcecopy in separate operations.\n");
        Destroy(argc, argv, -1);
    }

    
    if (g_bPrintDirContents)
    {
        if(!g_bQuiet)
            g_pResourceCopy->GenerateHardwareReport(g_szSourceDir, g_szSourceDir, true);

        g_pResourceCopy->PrintDirContents(g_szSourceDir);
    }
    else if (g_bDeleteDir)
    {
        if (!g_bQuiet)
            g_pResourceCopy->GenerateHardwareReport(g_szDeleteDirPath, g_szDeleteDirPath, true);

        g_pResourceCopy->DeleteDirRecursive(g_szDeleteDirPath, true);
    }
    else if (!g_bSingleFile && g_bTransferFiles)
    {
        if (!g_bQuiet)
            g_pResourceCopy->GenerateHardwareReport(g_szSourceDir, g_szDestinationDir, true);

        g_pResourceCopy->TransferDirTo(g_szSourceDir, g_szDestinationDir, true, g_bOverWriteFiles);
    }
    else if(g_bSingleFile)
    {
        if (!g_bQuiet)
            g_pResourceCopy->GenerateHardwareReport(g_szSourceDir, g_szDestinationDir, false);

        Msg("---- Operation: %s ----\n", g_bTransferFiles ? "Copying" : "Trasfering");

        char szDstFile[MAX_PATH];
        char* ptr = V_strrchr(g_szSourceDir, '\\') + 1;
        V_snprintf(szDstFile, sizeof(szDstFile), "%s%s", g_szDestinationDir, ptr);

        Msg("Source: ");        ColorSpewMessage(SPEW_MESSAGE, &ColorPath, "%s\n", g_szSourceDir);
        Msg("Destination: ");   ColorSpewMessage(SPEW_MESSAGE, &ColorPath, "%s\n", szDstFile);
        Msg("\n");

        if(!g_pResourceCopy->FileExist(g_szSourceDir))
        {
            ColorSpewMessage(SPEW_MESSAGE, &ColorUnSucesfull, "File: %s, doesnt exist!\n", g_szSourceDir);
            Destroy(argc, argv, -1);
        }

        if(!g_pResourceCopy->CreateDir(g_szDestinationDir))
        {
            ColorSpewMessage(SPEW_MESSAGE, &ColorUnSucesfull, "Failed to create dir at: %s\n", g_szDestinationDir);
            g_pResourceCopy->GenerateErrorReport();
            Destroy(argc, argv, -1);
        }

        if (g_bTransferFiles) 
        {
            if (!g_pResourceCopy->TransferFileTo(g_szSourceDir, szDstFile, g_bOverWriteFiles))
            {
                ColorSpewMessage(SPEW_MESSAGE, &ColorUnSucesfull, "Failed to transfer file at: %s\n", g_szDestinationDir);
                g_pResourceCopy->GenerateErrorReport();
                Destroy(argc, argv, -1);
            }
        }
        else
        {
            if (!g_pResourceCopy->CopyFileTo(g_szSourceDir, szDstFile, g_bOverWriteFiles))
            {
                ColorSpewMessage(SPEW_MESSAGE, &ColorUnSucesfull, "Failed to copy file at: %s\n", g_szDestinationDir);
                g_pResourceCopy->GenerateErrorReport();
                Destroy(argc, argv, -1);
            }
        }

        Msg("Coping file: "); ColorSpewMessage(SPEW_MESSAGE, &ColorPath, "%s", g_szSourceDir); Msg(" -> "); ColorSpewMessage(SPEW_MESSAGE, &ColorPath, "%s", szDstFile);
    }
    else // Normal copy mode 
    {
        if (!g_bQuiet)
            g_pResourceCopy->GenerateHardwareReport(g_szSourceDir, g_szDestinationDir, true);

        g_pResourceCopy->CopyDirTo(g_szSourceDir, g_szDestinationDir, true, g_bOverWriteFiles);
    }

    Destroy(argc, argv);
    return 0;
}

