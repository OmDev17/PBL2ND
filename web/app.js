// Global state
let projectsList = [];

// Initialize
document.addEventListener('DOMContentLoaded', () => {
  loadSummary();
});

// Switch Tabs
function openTab(tabId) {
  document.querySelectorAll('.menu-btn').forEach(b => b.classList.remove('active'));
  document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));

  event.target.classList.add('active');
  document.getElementById(tabId).classList.add('active');

  if (tabId === 'tab-summary') loadSummary();
  if (tabId === 'tab-compare' || tabId === 'tab-inspect') populateDropdowns();
}

// 1. Analyze Code Snippet
async function analyzeCode() {
  const project = document.getElementById('project-name').value.trim() || 'UntitledProject';
  const code = document.getElementById('code-input').value.trim();
  const btn = document.getElementById('btn-analyze');
  const outBox = document.getElementById('analyze-output');
  const savedMsg = document.getElementById('out-saved');

  if (!code) {
    alert('Please enter a code snippet first!');
    return;
  }

  btn.disabled = true;
  btn.textContent = 'Analyzing...';

  try {
    const res = await fetch('/api/analyze', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ project, code })
    });
    const data = await res.json();

    if (!data.success) {
      alert('Error: ' + (data.error || 'Unable to analyze input.'));
      return;
    }

    outBox.style.display = 'block';
    document.getElementById('out-bigo').textContent = data.bigO;
    document.getElementById('out-rank').textContent = data.priorityRank;
    document.getElementById('out-loops').textContent = data.loopCount;
    document.getElementById('out-depth').textContent = data.maxLoopDepth;
    document.getElementById('out-direct').textContent = data.hasDirectRecursion ? 'Yes' : 'No';
    document.getElementById('out-indirect').textContent = data.hasIndirectRecursion ? 'Yes' : 'No';
    document.getElementById('out-rectype').textContent = data.recursionType || 'None';

    if (data.saved) {
      savedMsg.style.display = 'block';
      setTimeout(() => { savedMsg.style.display = 'none'; }, 4000);
    }

    loadSummary();
  } catch (err) {
    alert('Network error connecting to C++ engine.');
  } finally {
    btn.disabled = false;
    btn.textContent = 'Analyze & Save';
  }
}

// 2. Load Summary Table
async function loadSummary() {
  const tbody = document.getElementById('summary-rows');
  tbody.innerHTML = '<tr><td colspan="5">Loading submissions...</td></tr>';

  try {
    const res = await fetch('/api/summary');
    const data = await res.json();

    projectsList = data.projects || [];

    if (projectsList.length === 0) {
      tbody.innerHTML = '<tr><td colspan="5">No saved projects found in submissions.txt.</td></tr>';
      return;
    }

    tbody.innerHTML = '';
    projectsList.forEach((p, idx) => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td><strong>${escape(p.name)}</strong></td>
        <td>${p.submissions}</td>
        <td>${escape(p.lastBigO)}</td>
        <td><strong>${escape(p.optimalBigO)}</strong></td>
        <td>
          <button class="btn btn-small" onclick="viewSummaryCode(${idx})">View Code</button>
        </td>
      `;
      tbody.appendChild(tr);
    });

    populateDropdowns();
  } catch (err) {
    tbody.innerHTML = '<tr><td colspan="5">Failed to load data.</td></tr>';
  }
}

// Show Source Code inside the Summary Tab
function viewSummaryCode(idx) {
  const p = projectsList[idx];
  if (!p) return;

  const box = document.getElementById('summary-code-box');
  const title = document.getElementById('summary-code-title');
  const meta = document.getElementById('summary-code-meta');
  const codeEl = document.getElementById('summary-code-content');

  box.style.display = 'block';
  title.textContent = `Source Code: ${p.name}`;
  meta.textContent = `Last Recorded Big-O: ${p.lastBigO} | Optimal Big-O: ${p.optimalBigO} | Timestamp: ${p.lastTimestamp || 'N/A'}`;
  codeEl.textContent = p.lastCode || p.optimalCode || '// No code recorded.';

  box.scrollIntoView({ behavior: 'smooth' });
}

function populateDropdowns() {
  const p1 = document.getElementById('cmp-p1');
  const p2 = document.getElementById('cmp-p2');
  const insp = document.getElementById('insp-project');

  [p1, p2, insp].forEach(sel => {
    if (!sel) return;
    const curr = sel.value;
    sel.innerHTML = '';
    projectsList.forEach(p => {
      const opt = document.createElement('option');
      opt.value = p.name;
      opt.textContent = `${p.name} (${p.submissions} submissions)`;
      sel.appendChild(opt);
    });
    if (curr) sel.value = curr;
  });

  if (p2 && projectsList.length > 1 && !p2.value) {
    p2.selectedIndex = 1;
  }
}

// 3. Compare Two Projects
async function compareProjects() {
  const p1 = document.getElementById('cmp-p1').value;
  const p2 = document.getElementById('cmp-p2').value;

  if (!p1 || !p2) {
    alert('Please select two projects to compare.');
    return;
  }

  try {
    const res = await fetch('/api/compare', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ p1, p2 })
    });
    const data = await res.json();

    if (!data.success) {
      alert(data.error || 'Failed to compare.');
      return;
    }

    const box = document.getElementById('compare-box');
    box.style.display = 'block';

    document.getElementById('cmp-details').innerHTML = `
      <strong>[1] ${escape(data.p1.name)}:</strong> ${escape(data.p1.bigO)} (Rank ${data.p1.rank})<br>
      <strong>[2] ${escape(data.p2.name)}:</strong> ${escape(data.p2.bigO)} (Rank ${data.p2.rank})
    `;
    document.getElementById('cmp-winner').textContent = 'Result: ' + data.verdict;
  } catch (err) {
    alert('Comparison error.');
  }
}

// 4 & 5. Inspect Solution
async function fetchSolution() {
  const name = document.getElementById('insp-project').value;
  const mode = document.getElementById('insp-mode').value;

  if (!name) return;

  try {
    const res = await fetch(`/api/project?name=${encodeURIComponent(name)}&mode=${mode}`);
    const data = await res.json();

    if (!data.success) {
      alert(data.error || 'Record not found.');
      return;
    }

    const box = document.getElementById('inspect-box');
    box.style.display = 'block';

    document.getElementById('insp-time').textContent = data.timestamp;
    document.getElementById('insp-bigo').textContent = data.bigO;
    document.getElementById('insp-rank').textContent = data.priorityRank;
    document.getElementById('insp-code').textContent = data.sourceCode;
  } catch (err) {
    alert('Failed to fetch solution.');
  }
}

// 6. Run Demo Test Case
async function runDemo(demoId) {
  try {
    const res = await fetch('/api/demo', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ demoId })
    });
    const data = await res.json();

    if (!data.success) {
      alert(data.error || 'Demo execution failed.');
      return;
    }

    const box = document.getElementById('demo-box');
    box.style.display = 'block';

    document.getElementById('demo-title').textContent = `${data.demoName} (${data.file})`;
    document.getElementById('demo-bigo').textContent = data.bigO;
    document.getElementById('demo-rank').textContent = data.priorityRank;
    document.getElementById('demo-code').textContent = data.sourceCode;

    loadSummary();
  } catch (err) {
    alert('Failed to run demo.');
  }
}

function escape(s) {
  if (!s) return '';
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}
