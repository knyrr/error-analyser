#include <iostream>
#include <fstream>
#include <sstream> 
#include <string>
#include <vector>
#include <map> 
using namespace std;

typedef pair<int, int> Key;

enum UnitStatus { initial, added, modifiedSource, modifiedTarget, deleted };
struct Unit {
  string content;
  UnitStatus status = initial;
  int variant = -1;
};

typedef multimap<Key, Unit> SentenceUnits;

struct ModType {
  string code;
  vector<string> content;
};

struct Annotation {
  Key position;
  ModType type;
  string content;
  string required;
  string member;
  int variant;
};

struct SentenceWithAnnotation {
  SentenceUnits content;
  vector<SentenceUnits> modifiedContent;
  vector<Annotation> annotations;
};

void printSentence(SentenceUnits content){
  string target;
  for (const auto& unit : content) {
    if (unit.second.status == initial || unit.second.status == added || unit.second.status == modifiedTarget ){
      target = target + unit.second.content + " ";
    } 
  }
  target.pop_back();
  cout << target << endl;
}

void printSentences(vector<SentenceWithAnnotation> sentences){
  for(SentenceWithAnnotation sentence : sentences){
    printSentence(sentence.content);
    for(SentenceUnits modifiedSentence : sentence.modifiedContent){
      cout << "A: ";
      printSentence(modifiedSentence);
    }
    cout << endl;
  }
}

vector<string> sliceContent(string input, char delim = ' '){
  vector<string> content;
  string segment;
  stringstream strStream(input);
  while (getline(strStream, segment, delim)) {
    if(!segment.empty()){
      content.push_back(segment);
      // cout << segment << endl;
    } 
  }
  return content;
};

SentenceUnits transformSentence(string input){
  SentenceUnits sentence;
  vector<string> slicedContent = sliceContent(input);
  Key position;
  for (int i = 0; i < slicedContent.size(); i++) {
    position = make_pair(i, i + 1);
    Unit unit;
    unit.content = slicedContent[i];
    unit.status = initial;
    sentence.insert(std::make_pair(position, unit));
  } 
  return sentence;
}

Annotation transformAnnotation(string input){
  Annotation annotation;
  vector<string> slicedAnnotation = sliceContent(input, '|');

  vector<string> slicedPosition = sliceContent(slicedAnnotation[0]);
  int start = stoi(slicedPosition[0]);
  int end = stoi(slicedPosition[1]);
  Key position (start, end);
  annotation.position = position; 

  vector<string> slicedType = sliceContent(slicedAnnotation[1], ':');
  annotation.type.code = slicedType[0];
  vector<string> typeContent;
  for (int i = 1; i < slicedType.size(); i++) {
    typeContent.push_back(slicedType[i]);
  }
  annotation.type.content = typeContent;
  
  annotation.content = slicedAnnotation[2];
  annotation.required = slicedAnnotation[3];
  annotation.member = slicedAnnotation[4];
  annotation.variant = stoi(slicedAnnotation[5]);
  return annotation;
}

SentenceWithAnnotation applyAnnotation(SentenceWithAnnotation sentenceWithAnnotation){
  SentenceUnits sentence = sentenceWithAnnotation.content;
  SentenceWithAnnotation modifiedSentence = sentenceWithAnnotation;

  for(Annotation annotation : sentenceWithAnnotation.annotations){

    if (annotation.variant != modifiedSentence.modifiedContent.size()){
      modifiedSentence.modifiedContent.push_back(sentence);
      sentence = modifiedSentence.content;
    }
    
    //replace
    if (annotation.type.code == "R") { 
      vector<string> annotationContent = sliceContent(annotation.content);
      int start = annotation.position.first;
      int end = annotation.position.second;
      Key position;

      for (int i = 0; i < end-start; i++) {
        position = make_pair(start + i, start + i + 1);
        auto initalUnit = sentence.find(position);
        initalUnit->second.status = modifiedSource;
      } 

      Unit target;
      target.content = annotation.content;
      target.status = modifiedTarget;
      target.variant = annotation.variant; 
      sentence.insert(std::make_pair(annotation.position, target));
    }
    //missing
    if (annotation.type.code == "M") { 
      Unit target;
      target.content = annotation.content;
      target.status = added;
      target.variant = annotation.variant;
      sentence.insert(std::make_pair(annotation.position, target));
    }
    //delete
    if (annotation.type.code == "U") { 
      auto initalUnit = sentence.find(annotation.position);
        initalUnit->second.status = deleted;
    }
   }
  modifiedSentence.modifiedContent.push_back(sentence);
  return modifiedSentence;
}

vector<SentenceWithAnnotation> applyAnnotationForAll(vector<SentenceWithAnnotation> sentences){
  vector<SentenceWithAnnotation> modifiedSentences;
  for(SentenceWithAnnotation sentence : sentences){
      SentenceWithAnnotation modifiedSentence = applyAnnotation(sentence);
      modifiedSentences.push_back(modifiedSentence);
  }
  return modifiedSentences;
}

void printCommonChanges(SentenceWithAnnotation sentence){
  if(sentence.modifiedContent.size()>0){
    SentenceUnits mergedSentence = sentence.modifiedContent[0]; 

    for (int i = 0; i < sentence.modifiedContent.size(); i++) {
      SentenceUnits _mergedSentence;
      for (auto& kv : sentence.modifiedContent[i]) {
        auto range = mergedSentence.equal_range(kv.first);
        for (auto i = range.first; i != range.second; ++i) {
            if (i->second.content == kv.second.content && i->second.status == kv.second.status) {
            _mergedSentence.insert(kv);
          }
        }
      }
      mergedSentence = _mergedSentence;
    }

    printSentence(sentence.content);
    for(SentenceUnits modifiedSentence : sentence.modifiedContent){
      cout << "A: ";
      printSentence(modifiedSentence);
    }
    printSentence(mergedSentence);
  }
}


vector<SentenceWithAnnotation> readFile(string fileName) {
  vector<SentenceWithAnnotation> sentences;
  SentenceWithAnnotation sentence;
  
  ifstream file(fileName);
  string line;
  while (getline(file, line)) { 
    if (line[0]=='S'){
      if (!sentence.content.empty()) {
        sentences.push_back(sentence);
      }
      sentence.content = transformSentence(line.erase(0, 2));
      sentence.annotations.clear();
    } else if (line[0]=='A'){
      Annotation annotation = transformAnnotation(line.erase(0, 2));
      sentence.annotations.push_back(annotation);
    }
  }
  sentences.push_back(sentence);
  return sentences;
}

int main(){
  string fileName = "A2III_003-007.txt";
  vector<SentenceWithAnnotation> sentences;
  sentences = readFile(fileName);
  sentences = applyAnnotationForAll(sentences);
  // printSentences(sentences);
  printCommonChanges(sentences[2]);
  
  return 0;
}
