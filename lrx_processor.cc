/*
 * Copyright (C) 2011 Universitat d'Alacant
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <lrx_processor.h>

using namespace std;

wstring const LRXProcessor::LRX_PROCESSOR_S_BOUNDARY = L".<sent>";

wstring
LRXProcessor::itow(int i)
{
  // Convert an int to a wstring
  wchar_t buf[50];
  memset(buf, '\0', sizeof(buf));
  swprintf(buf, 50, L"%d", i);
  wstring id(buf);
  return id;
}

int 
LRXProcessor::wtoi(wstring w)
{
  // Convert a wstring to an int
  wistringstream wstrm(w);
  int i_name = -655;
  wstrm >> i_name;

  return i_name;
}

LRXProcessor::LRXProcessor()
{
  pool = new Pool<vector<int> >(4, vector<int>(50));

  initial_state = new State(pool);
  current_state = new State(pool);

  traceMode = false;
  pos = 0;
}

LRXProcessor::~LRXProcessor()
{
  delete current_state;
  delete initial_state;
  delete pool;
}

void
LRXProcessor::setTraceMode(bool m)
{
  traceMode = m;
}
void
LRXProcessor::load(FILE *in)
{
  // Read in the alphabet 

  alphabet.read(in);

  // Read in the individual patterns

  int len = Compression::multibyte_read(in);

  while(len > 0)
  {
    int len2 = Compression::multibyte_read(in);
    wstring name = L"";
    while(len2 > 0)
    {
      name += static_cast<wchar_t>(Compression::multibyte_read(in));
      len2--;
    }
    int i_name = wtoi(name);
    Transducer t;
    t.read(in);
/*
    map<int, int> finals;
    for(int i = 0; i < t.size(); i++) 
    { 
      if(!t.isFinal(i))
      {
        continue;
      }
      finals[i] = i;
    } 
    MatchExe me(t, finals);
    patterns[i_name] = me;
*/
    patterns[i_name] = t;
    len--;
  }

  fwprintf(stderr, L"Patterns: %d, Alphabet: %d\n", patterns.size(), alphabet.size());

  // Now read in the main rule pattern transducer

  int len3 = Compression::multibyte_read(in);

  wstring name = L"";
  while(len3 > 0)
  {
    name += static_cast<wchar_t>(Compression::multibyte_read(in));
    len3--;
  }

  // For debugging
  //Transducer v;
  //v.read(in);
  //v.show(alphabet, stderr);

  transducer.read(in, alphabet);

  // Now read in the rule records, for lengths/weights

  while(!feof(in))
  {
    LSRuleExe rec;
    fread(&rec, sizeof(LSRuleExe), 1, in);
    //fwprintf(stderr, L"%d len(%d) weight(%f)\n", rec.id, rec.len, rec.weight);
    rules[rec.id] = rec;
  }

  //fclose(in);
 
  return;
}

void 
LRXProcessor::init()
{
  initial_state->init(transducer.getInitial());

  anfinals.insert(transducer.getFinals().begin(), transducer.getFinals().end());
}

vector<int>
LRXProcessor::pathsToRules(wstring const path)
{
  vector<wstring> matched_paths;
  vector<int> matched_rules;
 
  wstring loc_buf = L"";
  for(wstring::const_iterator it = path.begin(); it != path.end(); it++)
  {
    if(*it == L'/') 
    { 
      matched_paths.push_back(loc_buf);
      loc_buf = L"";
    }
    loc_buf = loc_buf + *it;
  }
  if(loc_buf.compare(L"") != 0)
  {
    matched_paths.push_back(loc_buf);
  } 

  for(vector<wstring>::iterator it = matched_paths.begin(); it != matched_paths.end(); it++)
  {
    wstring wm = *it;
    wstring id_buf = L"";
    for(wstring::iterator it2 = wm.end(); it2 != wm.begin(); it2--)
    {
      if(*it2 == L'>')
      { 
        continue;
      }
      if(*it2 == L'<')
      {
        break;
      }
      id_buf = id_buf + *it2;
    }
    if(id_buf.compare(L"") != 0) 
    { 
      reverse(id_buf.begin(), id_buf.end());
      int p_id = wtoi(id_buf);
      matched_rules.push_back(p_id); 
    }
  }
    
  return matched_rules;
}

void 
LRXProcessor::applyRules(map<int, SItem> &sentence, FILE *output)
{
  map< pair<int, int>, vector<int> > rule_spans;
  vector<State> alive_states;
  vector<State> current_states;
  alive_states.push_back(*initial_state);
  current_states.push_back(*initial_state);

  fwprintf(stderr, L"applyRules (%d):\n", sentence.size());

  int j = 0; // current range start
  int k = 0; // current range end

  for(unsigned int i = 0; i < sentence.size(); i++)
  //for(map<int, SItem>::iterator it = sentence.begin(); it != sentence.end(); it++)
  {
    SItem s = sentence[i];
    fwprintf(stderr, L"%d [%d:%d] %S(%d) (A: %d) (C: %d)\n", i, j, k, s.sl.c_str(), s.tl.size(), alive_states.size(), current_states.size());
/*
    for(vector<State>::const_iterator it2 = alive_states.begin(); it2 != alive_states.end(); it2++) 
    {
*/
      vector<int> rules;
      //fwprintf(stderr, L"%d %S %d\n", it->first, it->second.sl.c_str(), it->second.tl.size());
      //State s = *it2;
      State is = *initial_state;
      for(k = j; k < sentence.size(); k++)
      {
        if(is.size() == 0) 
        { 
          break;
        }
        if(is.isFinal(anfinals))
        {
          wstring out = is.filterFinals(anfinals, alphabet, escaped_chars);
  
          fwprintf(stderr, L"%d->%d : %S\n", j, k, out.c_str());
   
          pair<int, int> span = make_pair(j, k);
          rule_spans[span] = pathsToRules(out);
          //j = k; 
          //rules.clear();
        }
        is.step(sentence[k].sl, patterns, alphabet, stderr);
      }
      k = j;
/*
    }
    current_states.push_back(*initial_state);
    alive_states = current_states;
*/
    j = i;
  }
  
  for(map< pair<int, int>, vector<int> >::iterator it = rule_spans.begin(); it != rule_spans.end(); it++) 
  {
    pair<int, int> span = it->first;
    vector<int> rules = it->second;
    fwprintf(stderr, L"%d:%d = ", span.first, span.second);
    for(vector<int>::iterator it2 = rules.begin(); it2 != rules.end(); it2++)
    {
      fwprintf(stderr, L"%d ", *it2);
    }
    fwprintf(stderr, L"\n");
    
  }

  return;
}

void 
LRXProcessor::readWord(SItem &w, FILE *input, FILE *output)
{
  int val = 0;
  bool first = true;
  wstring tl = L"";

  val = fgetwc_unlocked(input);
  while(val != L'$')
  {
    if(val == L'\\')
    {
      if(first)
      {
        w.sl += static_cast<wchar_t>(val);
        val = fgetwc_unlocked(input);
        w.sl += static_cast<wchar_t>(val);
        val = fgetwc_unlocked(input);
        continue;
      }
      else
      { 
        tl += static_cast<wchar_t>(val);
        val = fgetwc_unlocked(input);
        tl += static_cast<wchar_t>(val);
        val = fgetwc_unlocked(input);
        continue;
      }
    }

    if(val == L'/' && first)
    {
      first = false;
      val = fgetwc_unlocked(input);
      continue;
    }
    else if(val == L'/')
    {
      w.tl.push_back(tl);
      tl = L"";
      val = fgetwc_unlocked(input);
      continue;
    }

    if(first)
    {
      w.sl += static_cast<wchar_t>(val); 
    }
    else
    {
      tl += static_cast<wchar_t>(val); 
    }

    val = fgetwc_unlocked(input);
  }
  w.tl.push_back(tl);
}


void
LRXProcessor::process(FILE *input, FILE *output)
{
  map<int, SItem> sentence;   // pos, item
  bool isEscaped = false;

  sentence.clear();

  int val = fgetwc_unlocked(input);
  while(!feof(input))
  {
    if(val == L'\\')
    {
      isEscaped = true;
      sentence[pos].blank += static_cast<wchar_t>(val);
      val = fgetwc_unlocked(input);
      sentence[pos].blank += static_cast<wchar_t>(val);
      val = fgetwc_unlocked(input);
      isEscaped = false;
    }

    if(val == L'^' && !isEscaped && outOfWord)
    { 
      outOfWord = false;
      readWord(sentence[pos], input, output);
      pos++;
    } 

    if(sentence[pos-1].sl.compare(LRX_PROCESSOR_S_BOUNDARY) == 0)
    {
      applyRules(sentence, output);
      for(map<int, SItem>::iterator it = sentence.begin(); it != sentence.end(); it++)
      { 
        SItem w = it->second;
        //fwprintf(stderr, L"sentence[%d]: %S\n", it->first, w.sl.c_str());
        fputws_unlocked(w.blank.c_str(), output);
        if(w.sl != L"") 
        { 
          fputws_unlocked(L"^", output);
          fputws_unlocked(w.sl.c_str(), output);
          for(vector<wstring>::iterator it2 = w.tl.begin(); it2 != w.tl.end(); it2++) 
          {
            if(it2 != w.tl.end()) 
            {
              fputws_unlocked(L"/", output);
            }
            fputws_unlocked(it2->c_str(), output);
          }
          fputws_unlocked(L"$", output);
        }
      }
      sentence.clear();
      pos = 0;
    }

    if(outOfWord)
    {
      sentence[pos].blank += static_cast<wchar_t>(val);
    }

    outOfWord = true;

    val = fgetwc_unlocked(input);
  }      
  //fwprintf(stderr, L"sentence[%d]: %S\n", pos, sentence[pos].sl.c_str());
  fputws_unlocked(sentence[pos].blank.c_str(), output);
  if(sentence[pos].sl != L"") 
  { 
    fputws_unlocked(L"^", output);
    fputws_unlocked(sentence[pos].sl.c_str(), output);
    for(vector<wstring>::iterator it2 = sentence[pos].tl.begin(); it2 != sentence[pos].tl.end(); it2++) 
    {
      if(it2 != sentence[pos].tl.end()) 
      {
        fputws_unlocked(L"/", output);
      }
      fputws_unlocked(it2->c_str(), output);
    }
    fputws_unlocked(L"$", output);
  }

  return;
}


