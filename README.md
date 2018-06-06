![](https://api.travis-ci.org/approach0/search-engine.svg)
![](http://github-release-version.herokuapp.com/github/approach0/search-engine/release.png)

![](https://raw.githubusercontent.com/approach0/search-engine-docs-eng/master/logo.png)

Approach0 is a math-aware search engine.

Please visit [https://approach0.xyz/demo](https://approach0.xyz/demo) for a WEB demo (not using the new model yet).

![](https://github.com/approach0/search-engine-docs-eng/raw/master/img/clip.gif)

This branch is saved for reproducing `CIKM-2018` paper results. Sections below describes how to setup,
index and search NTCIR-12 dataset using Approach0. This branch also provides some tool scripts for generating TREC format files.

Details on our evaluation results (including query-by-query results) and also the NTCIR-12 dataset (in LaTeX)
can be viewed here:
[http://approach0.xyz:3838](http://approach0.xyz:3838)

More technical documentation is available here:
[https://approach0.xyz/docs](https://approach0.xyz/docs)

## Build (Ubuntu system)
Clone the CIKM-2018 branch
```
git clone -b cikm2018 --single-branch https://github.com/approach0/search-engine 
cd search-engine
PROJECT=`pwd`
```

Install external dependencies:
```
sudo apt-get update
sudo apt-get install g++ cmake
sudo apt-get install bison flex libz-dev libevent-dev
```

Download and build Indri (only used for text search, but we still have to build it) to your home directory
```
cd ~
wget 'https://sourceforge.net/projects/lemur/files/lemur/indri-5.11/indri-5.11.tar.gz/download' -O indri.tar.gz
tar -xzf indri.tar.gz
(cd indri-5.11 && chmod +x configure && ./configure && make)
```

Dowload CppJieba (again, not used in math search, but it is required for building the system)
```
cd ~
wget 'https://github.com/yanyiwu/cppjieba/archive/v4.8.1.tar.gz' -O cppjieba.tar.gz
mkdir -p ~/cppjieba
tar -xzf cppjieba.tar.gz -C ~/cppjieba --strip-components=1
```

Configure and check dependencies
```
cd $PROJECT
./configure
```

you need to run the `configure` script successfully without reporting any missing library before proceed to `make`:
```
make
```
## Indexing
Before indexing, first you need to setup a index daemon,
```
cd indexer
./run/indexd.out
```

Go to a new terminal session, download the NTCIR-12 math browsing task corpus and feed it to the index daemon:
```
cd $PROJECT/indexer/test-corpus/ntcir12
wget http://approach0.xyz:3838/trecfiles/corpus.txt
mv corpus.txt ntcir12.tmp
./ntcir-feeder.py
```

The indexing takes quite a while, we strongly suggest running on a SSD disk because our current indexer produces a lot of random writes. When indexing, `ntcir-feeder.py` script will show the progress (lines of input file processed), the script exits when indexing is finished. At this point, you will need to use `Ctrl` + `C` to terminate index daemon before searching on the fresh index. A `tmp` file is created at the directory of `indexer`, it is the generated index.

## Search and generate TREC output

## License
MIT

## Contact
zhongwei@udel.edu

Thanks for the [DPRL lab](https://www.cs.rit.edu/~rlaz/) for supporting my research.

![](https://www.cs.rit.edu/~rlaz/images/DPRL_Logo_Option_02.png)
