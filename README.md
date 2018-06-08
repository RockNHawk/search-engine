![](https://api.travis-ci.org/approach0/search-engine.svg)
![](http://github-release-version.herokuapp.com/github/approach0/search-engine/release.png)

![](https://raw.githubusercontent.com/approach0/search-engine-docs-eng/master/logo.png)

Approach0 is a math-aware search engine.

WEB demo: [https://approach0.xyz/demo](https://approach0.xyz/demo) (not using the new model yet).

![](https://github.com/approach0/search-engine-docs-eng/raw/master/img/clip.gif)

This branch is saved for reproducing results. Sections below describes how to setup,
index and search NTCIR-12 dataset using Approach0. This branch also provides some tool scripts for generating TREC format files.

## Build (Ubuntu system)
Clone this branch
```
git clone -b <branch_name> --single-branch https://github.com/approach0/search-engine 
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
Before indexing, first you need to setup an index daemon,
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

The indexing takes quite a while, we strongly suggest running on a SSD disk because our current indexer naively produces a lot of random writes.
When indexing, `ntcir-feeder.py` script will show the progress (lines of input file processed), the script exits when indexing is finished.
At this point, you will need to use `Ctrl` + `C` to terminate index daemon before searching on the fresh index. The output directory `tmp` is created at the directory of `indexer`, it is the generated index. Please delete the entire `tmp` directory everytime you need to re-index a new corpus.

## Search and generate TREC output
Before evaluating NTCIR-12 queries, you can experiment your own query on the newly built index.

A test-search program is provided for trying out some queries:
```
cd $PROJECT
./search/run/test-search.out -n -i ./indexer/tmp/ -m 'a+b' -q 'test-query-ID'
```
Each run of test-search program will append TREC format results to the log file named `trec-format-results.tmp`.

Download NTCIR-12 query data and exclude wildcards queries (wildcards are not yet supported in current system):
```
cd $PROJECT
wget http://approach0.xyz:3838/trecfiles/topics.txt
cat topics.txt | awk '{split($1,a,"-"); if (a[3] <= 20) print $0}' > ./topics-concrete.tmp
```

Now simply run `./genn-trec-results.py` to invoke test-search program and concatenate all the TREC output for all queries.
```
./genn-trec-results.py 2> runtime.log
```
The wall-clock runtime is written into `runtime.log`.

## Evaluation
Download trec-eval and build it:
```
git clone https://github.com/usnistgov/trec_eval
cd trec_eval
make
ln -sf `pwd`/trec_eval $PROJECT
```

Download NTCIR-12 judgement data:
```
cd $PROJECT
wget http://approach0.xyz:3838/trecfiles/judge.dat
mv judge.dat NTCIR12_MathWiki-qrels_judge.dat
```

Run `eval-trec-results.sh` to evaluate `trec-format-results.tmp` file just generated:
```
./eval-trec-results.sh trec-format-results.tmp
```

## License
MIT

## Contact
zhongwei@udel.edu

Thanks for the [DPRL lab](https://www.cs.rit.edu/~rlaz/) for supporting my research.

![](https://www.cs.rit.edu/~rlaz/images/DPRL_Logo_Option_02.png)
