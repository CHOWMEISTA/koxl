#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
using namespace std;
namespace fs = std::filesystem;

static string trim(const string &s){
    size_t a = s.find_first_not_of(" \t\r\n");
    if(a==string::npos) return string();
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b-a+1);
}

// Very small translator: supports lines like: print("text");
// Generates a C source, assembles to object (.o) using clang, and places it in dist/bin/1.0/<name>.o

static pair<string,string> parse_kxproj(const fs::path &proj){
    // returns {outputPath, outputName} (empty string if not found)
    string outPath, outName;
    if(!fs::exists(proj)) return {"", ""};
    ifstream ifs(proj);
    string xml((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
    auto find_tag = [&](const string &tag)->string{
        string open = "<" + tag + ">";
        string close = "</" + tag + ">";
        size_t a = xml.find(open);
        if(a==string::npos) return string();
        a += open.size();
        size_t b = xml.find(close, a);
        if(b==string::npos) return string();
        return trim(xml.substr(a, b-a));
    };
    outPath = find_tag("OutputPath");
    outName = find_tag("OutputName");
    if(outName.empty()) outName = find_tag("RootNamespace");
    if(outName.empty()) outName = proj.stem().string();
    return {outPath, outName};
}

int main(int argc, char** argv){
    if(argc<2){
        cerr<<"Usage: koxl [--scaffold [dir]] <source.kx>\n";
        return 2;
    }

    // scaffold mode: create minimal project skeleton
    if(string(argv[1])=="--scaffold"){
        fs::path target = fs::current_path();
        if(argc>=3) target = argv[2];
        fs::create_directories(target);
        // create Hello.kx
        ofstream hk(target / "Hello.kx");
        hk<<"// Hello.kx - sample KOXL source\n";
        hk<<"print(\"Hello, KOXL\");\n";
        hk.close();
        // create Hello.kxproj with OutputPath and OutputName
        ofstream kp(target / "Hello.kxproj");
        kp<<"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        kp<<"<Project ToolsVersion=\"Current\" DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
        kp<<"  <PropertyGroup>\n";
        kp<<"    <OutputName>Hello</OutputName>\n";
        kp<<"    <OutputPath>dist/bin/1.0</OutputPath>\n";
        kp<<"  </PropertyGroup>\n";
        kp<<"  <ItemGroup>\n";
        kp<<"    <Kx Include=\"Hello.kx\" />\n";
        kp<<"  </ItemGroup>\n";
        kp<<"  <Target Name=\"Build\">\n";
        kp<<"    <Exec Command=\"koxl %(Kx.Identity)\" />\n";
        kp<<"  </Target>\n";
        kp<<"</Project>\n";
        kp.close();
        // create README
        ofstream rd(target / "README.md");
        rd<<"KOXL minimal scaffold. Run `koxl Hello.kx` to compile."<<"\n";
        rd.close();
        cout<<"Scaffold created at "<<fs::absolute(target).string()<<"\n";
        return 0;
    }

    fs::path inpath = argv[1];
    if(!fs::exists(inpath)){
        cerr<<"File not found: "<<inpath.string()<<"\n";
        return 3;
    }
    string content;
    {
        ifstream ifs(inpath);
        content.assign(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());
    }

    vector<string> stmts;
    bool ok = true;
    size_t lineno = 0;
    istringstream iss(content);
    string line;
    while(getline(iss, line)){
        lineno++;
        string t = trim(line);
        if(t.empty()) continue;
        if(t.rfind("//",0) == 0) continue;
        char last = t.back();
        if(last=='{' || last=='}') continue;
        if(t.back()!=';'){
            cerr<<inpath.string()<<":"<<lineno<<": error: missing semicolon\n";
            ok = false;
        }
        stmts.push_back(t);
    }
    if(!ok) return 4;

    // Build C source
    fs::path tmpc = fs::temp_directory_path() / (inpath.stem().string() + string(".c"));
    ofstream ofs(tmpc);
    ofs<<"#include <stdio.h>\n";
    ofs<<"int main(){\n";
    for(auto &s: stmts){
        // support print("...");
        if(s.rfind("print(",0)==0){
            size_t a = s.find('(');
            size_t b = s.rfind(')');
            if(a!=string::npos && b!=string::npos && b>a+1){
                string inside = s.substr(a+1, b-a-1);
                // naive trim of surrounding quotes
                string t = trim(inside);
                if(t.size()>=2 && ((t.front()=='\"' && t.back()=='\"') || (t.front()=='\'' && t.back()=='\''))){
                    string lit = t.substr(1, t.size()-2);
                    ofs<<"    puts(\""<<lit<<"\");\n";
                    continue;
                }
            }
            // fallback: ignore
        }
        // other statements ignored for now
    }
    ofs<<"    return 0;\n";
    ofs<<"}\n";
    ofs.close();

    // determine project root by searching for nearest .kxproj upward from source
    fs::path proj_dir = inpath.parent_path();
    fs::path proj_file;
    while(!proj_dir.empty()){
        for(auto &p : fs::directory_iterator(proj_dir)){
            if(p.path().extension()==".kxproj"){ proj_file = p.path(); break; }
        }
        if(!proj_file.empty()) break;
        proj_dir = proj_dir.parent_path();
    }
    if(proj_dir.empty()) proj_dir = inpath.parent_path();

    // determine output path and name from .kxproj if present
    fs::path outdir;
    string outName;
    if(!proj_file.empty()){
        auto res = parse_kxproj(proj_file);
        string outPath = res.first;
        outName = res.second;
        if(!outPath.empty()){
            outdir = fs::path(proj_dir) / outPath;
        }
    }
    if(outdir.empty()) outdir = proj_dir / "dist" / "bin" / "1.0";
    fs::create_directories(outdir);
    if(outName.empty()) outName = inpath.stem().string();
    fs::path outobj = outdir / (outName + string(".o"));

    // Assemble using clang if available
    string cmd = "clang -c \"" + tmpc.string() + "\" -o \"" + outobj.string() + "\" 2>&1";
    int rc = system(cmd.c_str());
    if(rc!=0){
        cerr<<"Failed to assemble with clang. Command: "<<cmd<<"\n";
        return 5;
    }

    cout<<"OK: wrote object "<<outobj.string()<<"\n";
    return 0;
}
