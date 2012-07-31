#!/usr/bin/python
# coding=utf-8
# -*- encoding: utf-8 -*-

import sys, codecs, copy, commands;

sys.stdin  = codecs.getreader('utf-8')(sys.stdin);
sys.stdout = codecs.getwriter('utf-8')(sys.stdout);
sys.stderr = codecs.getwriter('utf-8')(sys.stderr);

# Read the corpus, make a note of all ambiguous words, their frequency and their possible translations

# sl_tl[sl_word][tl_word] = tl_freq

# Then we want to make a list of n-grams around the source words, with which target word they want, and the freq.

# ngrams[ngram][tl_word] = freq

# 5 	Please<vblex><inf> rise<n> ,<cm> then<adv> ,<cm> for<pr> this<det><dem> minute<n> 's<gen> silence<n> .<sent>
#5 	Please<vblex><inf>/Complacer<vblex><inf> rise<n><sg>/aumento<n><m><sg> ,<cm>/,<cm> then<adv>/entonces<adv> ,<cm>/,<cm> for<pr>/para<pr>/durante<pr> this<det><dem><sg>/este<det><dem><GD><sg> minute<n><sg>/minuto<n><m><sg> '<apos>/'<apos> *s/*s silence<n><sg>/silencio<n><m><sg> .<sent>/.<sent>
#5 	Invitar<vblex> a<pr> todo<prn><tn> a<pr> que<cnjsub> prpers<prn><pro> poner<vblex> de<pr> pie<n> para<pr> guardar<vblex><inf> uno<det><ind> minuto<n> de<pr> silencio<n> .<sent>
#5 	0-0 4-2 5-3 8-1 9-5 10-6 12-7 13-8 14-9 15-10
#-------------------------------------------------------------------------------

MAX_NGRAMS = 3;

cur_line = 0;

sl_tl = {};
ngrams = {};

cur_sl_row = [];
cur_tl_row = [];
cur_bt_row = [];
cur_al_row = [];

if len(sys.argv) < 2: #{
	print 'extract-freq-lexicon.py <candidate sent>';
	sys.exit(-1);
#}

for line in file(sys.argv[1]).readlines(): #{
	line = line.strip().decode('utf-8');	
	if line[0] == '-': #{
#		print len(cur_sl_row), len(cur_tl_row), len(cur_bt_row), len(cur_al_row);	
#		print cur_sl_row;
#		print cur_bt_row;
#		print cur_tl_row;
#		print cur_al_row;
#
		# Read the corpus, make a note of all ambiguous words, their frequency and their possible translations
		#
		# sl_tl[sl_word][tl_word] = tl_freq
		i = 0;
		for slword in cur_sl_row: #{
			if cur_bt_row[i].count('/') > 1: #{
				for al in cur_al_row: #{
					al_sl = int(al.split('-')[1]);
					al_tl = int(al.split('-')[0]);
					if al_sl != i: #{
						continue;
					#}
					tlword = cur_tl_row[al_tl].lower().split('>')[0] + '>';
					slword = slword.lower().split('>')[0] + '>';
					if slword not in sl_tl: #{
						sl_tl[slword] = {};
					#}
					if tlword not in sl_tl[slword]: #{
						sl_tl[slword][tlword] = 0;
					#}
					sl_tl[slword][tlword] = sl_tl[slword][tlword] + 1;

#					print '+' , slword , tlword , sl_tl[slword][tlword];
				#}

#				for j in range(0, MAX_NGRAMS): #{
#					print cur_sl_row[i-j:i+1];
#					print cur_sl_row[i:i+j];
#				#}
			#}	
			i = i + 1;
		#}

		cur_line = 0;
		#print line;	
		continue;
	#}	
	
	line = line.split('\t')[1];

	if cur_line == 0: #{
		cur_sl_row = line.split(' ');
	elif cur_line == 1: #{
		cur_bt_row = line.split(' ');
	elif cur_line == 2: #{
		cur_tl_row = line.split(' ');
	elif cur_line == 3:  #{
		cur_al_row = line.split(' ');
	#}

	cur_line = cur_line + 1;
#}

for sl in sl_tl: #{
	newtl = sorted(sl_tl[sl], key=lambda x: sl_tl[sl][x])
	newtl.reverse()
	first = True;
	for tl in newtl: #{
		if first: #{
			print sl_tl[sl][tl] , sl , tl , '@';
			first = False
		else: #{
			print sl_tl[sl][tl] , sl , tl;
		#}
	#}
#}