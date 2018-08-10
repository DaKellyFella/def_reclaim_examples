import sys, os, glob, re

import matplotlib.ticker as plticker
import matplotlib.pyplot as plt

from textwrap import wrap

def mean(numbers):
    return float(sum(numbers)) / max(len(numbers), 1)


# A map translating from the structure name to its 
# [category / nice name, lang, graphing style, and graphing colour]
structure_map = {
  'fhsl_lf' : {
    'structure_category' : 'Skip List',
    'lang': 'DEF'
  },
  'c_fhsl_lf' : {
    'structure_category' : 'Skip List',
    'lang': 'C'
  },
  'bt_lf' : {
    'structure_category' : 'Binary Tree',
    'lang': 'DEF'
  },
  'c_bt_lf' : {
    'structure_category' : 'Binary Tree',
    'lang': 'C'
  },
  'mm_ht' : {
    'structure_category' : 'Hash Table',
    'lang': 'DEF'
  },
  'c_mm_ht' : {
    'structure_category' : 'Hash Table',
    'lang': 'C'
  },
  'sl_pq' : {
    'structure_category' : 'Shavit Lotan Queue',
    'lang': 'DEF'
  },
  'c_sl_pq' : {
    'structure_category' : 'Shavit Lotan Queue',
    'lang': 'C'
  },
  'spray' : {
    'structure_category' : 'Spray List',
    'lang': 'DEF'
  },
  'c_spray' : {
    'structure_category' : 'Spray List',
    'lang': 'C'
  }
}

lang_map = {
  'DEF leaky' : {
    'graph_style' : '*', 
    'graph_colour': 'green'
  },
  'C leaky' : {
    'graph_style' : 's', 
    'graph_colour': 'blue'
  },
  'DEF retire' : {
    'graph_style' : '^', 
    'graph_colour': 'crimson'
  },
}

def collate_threads(data):
  thread_ops = {}
  for datum in data:
    (threads, ops) = datum
    if not(threads in thread_ops):
      thread_ops[threads] = []
    thread_ops[threads].append(ops)
  return thread_ops

def create_line_plots(config, perf_results):
  fig = plt.figure()
  (structure_name, lang_results) = perf_results[config]
  plt.title(config)
  plt.ylabel(r'Total Ops / $\mu$s')
  plt.xlabel('Thread Count')
  plt.xscale('linear')
  plt.autoscale(enable=True, axis='both', tight=None)
  plt.grid(b=True, which='major', color='black', linestyle='-')
  legends_list = []
  max_ops, min_ops = 0, 0
  for lang_config in lang_results:
    (policy, raw_name, data) = lang_results[lang_config]
    thread_ops = collate_threads(data)
    y_ticks = [0]
    y_error = [0]
    x_ticks = [0]
    for threads in range(1, max(thread_ops) + 1):
      if threads in thread_ops:
        y_ticks.append(mean(thread_ops[threads]))
        y_error.append(max(thread_ops[threads]) - min(thread_ops[threads]))
        x_ticks.append(threads)
    style_info = lang_map[lang_config]
    max_ops = max(max(y_ticks), max_ops)
    min_ops = min(min(y_ticks), min_ops)
    legends_list.append(
      plt.errorbar(x = x_ticks, y = y_ticks, yerr = y_error, linewidth=2,
        label = lang_config, color = style_info['graph_colour'], marker = style_info['graph_style'], markersize=10))
  # loc = plticker.MultipleLocator(base=0.5) # this locator puts ticks at regular intervals
  # plt.gca().yaxis.set_major_locator(loc)
  plt.legend(loc='best', fancybox=True, shadow=True, handles = legends_list, ncol = 3)
  
  fig.savefig('./figures/' + config + '.pdf', bbox_inches='tight')

def create_calibrating_bar_plots(set_results, pqueue_results):
  # All structures into one bar plot.
  def_results = {}
  c_results = {}
  for config in set_results:
    (structure_name, lang_results) = set_results[config]
    for lang_config in lang_results:
      (policy, raw_name, data) = lang_results[lang_config]
      thread_ops = collate_threads(data)
      lang = structure_map[raw_name]['lang']
      if lang == 'DEF' and policy == 'leaky':
        def_results[structure_name] = mean(thread_ops[1])
      else:
        c_results[structure_name] = mean(thread_ops[1])
  
  for config in pqueue_results:
    (structure_name, lang_results) = pqueue_results[config]
    for lang_config in lang_results:
      (policy, raw_name, data) = lang_results[lang_config]
      thread_ops = collate_threads(data)
      lang = structure_map[raw_name]['lang']
      if lang == 'DEF' and policy == 'leaky':
        def_results[structure_name] = mean(thread_ops[1])
      else:
        c_results[structure_name] = mean(thread_ops[1])
  
  structures = []
  def_numbers = []
  c_numbers = []
  for structure in def_results:
    structures.append('\n'.join(wrap(structure, 12)))
    def_numbers.append(def_results[structure])
    c_numbers.append(c_results[structure])
  
  def_norms = []
  c_norms = []
  for (def_res, c_res) in zip(def_numbers, c_numbers):
    def_norms.append(100.0)
    c_norms.append((c_res / def_res) * 100.0)
  fig = plt.figure()
  width = 0.35
  ind = range(len(def_norms))
  off_ind = [x +width for x in ind]
  plt.title("Normalised data-structure performance.")
  ax = plt.gca()
  bar1 = ax.bar(range(len(def_norms)), def_norms, width, color=lang_map['DEF leaky']['graph_colour'])
  bar2 = ax.bar(off_ind, c_norms, width, color=lang_map['C leaky']['graph_colour'])

  ax.set_xlim(-width,len(ind)+width)
  ax.set_ylim(0,130)
  ax.set_ylabel('Percentage')
  ax.set_title('C relative to DEF (both leaky)')
  ax.set_xticks([x +width for x in ind])
  xtickNames = ax.set_xticklabels(structures)
  plt.setp(xtickNames, rotation=45, fontsize=10)

  ## add a legend
  ax.legend( (bar1[0], bar2[0]), ('DEF', 'C') )
  plt.savefig('./figures/relativeperf.pdf', bbox_inches='tight')

def parse_file(key, data):
  key_file = key
  data_file = data

  keys = {}
  with open(key_file, "r") as open_file:
    for key_line in open_file:
      for idx, key in enumerate(key_line.split(",")):
        keys[key.strip()] = idx

  raw_results = []
  with open(data_file, "r") as open_file:
    for result in open_file:
      raw_results.append([x.strip() for x in result.split(",")])
  
  # Collate all the data.
  perf_results = {}
  for result in raw_results:
    # Get config.
    structure_key = result[keys['benchmark']]
    structure_m = structure_map[structure_key]
    structure_category = structure_m['structure_category']

    config = structure_category
    if 'update_rate' in keys:
      config += " w update rate: " + result[keys['update_rate']] + '%'

    if not(config in perf_results):
      perf_results[config] = (structure_category, {})
    _, category_results_map = perf_results[config]

    policy = result[keys['policy']]
    lang_config = structure_m['lang'] + ' ' + policy

    if not(lang_config in category_results_map):
      category_results_map[lang_config] = (policy, structure_key, [])
    _, _, lang_policy_results = category_results_map[lang_config]
    # Division to make into microseconds
    lang_policy_results.append((int(result[keys['threads']]), float(result[keys['ops/sec']]) / 1000000))
  return perf_results


def main():
    
  set_results = parse_file('set_keys.csv', 'set_data.csv')
  for config in set_results:
    create_line_plots(config, set_results)
  
  pqueue_results = parse_file('pqueue_keys.csv', 'pqueue_data.csv')
  for config in pqueue_results:
    create_line_plots(config, pqueue_results)
  
  create_calibrating_bar_plots(set_results, pqueue_results)
    
   
if __name__ == '__main__':
  main()