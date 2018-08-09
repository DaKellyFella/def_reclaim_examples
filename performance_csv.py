import sys, os, glob, re

import matplotlib.ticker as plticker
import matplotlib.pyplot as plt

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
    'graph_colour': 'blue'
  },
  'C leaky' : {
    'graph_style' : 's', 
    'graph_colour': 'green'
  },
  'DEF retire' : {
    'graph_style' : '^', 
    'graph_colour': 'crimson'
  },

}

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
    thread_ops = {}
    (policy, raw_name, data) = lang_results[lang_config]
    for datum in data:
      (threads, ops) = datum
      if not(threads in thread_ops):
        thread_ops[threads] = []
      thread_ops[threads].append(ops)
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

def create_calibrating_bar_plots(perf_results):
  # All structures into one bar plot.
  def_results = {}
  c_results = {}
  for config in perf_results:
    (structure_name, lang_results) = perf_results[config]
    for lang_config in lang_results:
      thread_ops = {}
      (policy, raw_name, data) = lang_results[lang_config]
      for datum in data:
        (threads, ops) = datum
        if threads == 1:
          print datum
  # Only 1 thread is necessary and only the leaky data-structures.


def main(args):
    
    key_file = args[1]
    data_file = args[2]

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
      lang_policy_results.append((int(result[keys['threads']]), int(result[keys['ops/sec']]) / 1000000))

    for config in perf_results:
      create_line_plots(config, perf_results)
    
    create_calibrating_bar_plots(perf_results)
    
   
if __name__ == '__main__':
    if len(sys.argv) != 3:
        exit(1)
    main(sys.argv)
