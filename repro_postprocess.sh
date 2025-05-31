# post process results
python3 process_results.py > repro_results.csv
echo "Results saved to repro_results.csv. Generating plots..."
python3 plot_results.py
echo "Plots generated in figs/"


