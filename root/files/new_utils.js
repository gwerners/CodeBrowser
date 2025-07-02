function renderResults() {
  const table = document.getElementById('resultsTable');
  for (const result of searchResults) {
    const dirRow = document.createElement('tr');
    dirRow.className = 'dir';
    const dirCell = document.createElement('td');
    dirCell.colSpan = 3;
    dirCell.innerHTML = `<a href="${result.url}">${result.dir}</a>`;
    dirRow.appendChild(dirCell);
    table.appendChild(dirRow);

    result.entries.forEach((entry, index) => {
      const row = document.createElement('tr');
      row.className = 'file-row';
      const fileCell = document.createElement('td');
      fileCell.className = 'f';
      fileCell.innerHTML = `<a href="${entry.url}">${entry.file}</a>`;

      const contentCell = document.createElement('td');
      contentCell.className = 'con';
      for (const line of entry.lines) {
        const a = document.createElement('a');
        a.className = 's';
        a.href = `${line.url}`;
        a.innerHTML = `<span class="l">${line.num}</span> ${line.content}`;
        contentCell.appendChild(a);
        contentCell.appendChild(document.createElement('br'));
      }

      row.appendChild(fileCell);
      row.appendChild(contentCell);
      table.appendChild(row);
    });
  }
}

function selectAllProjects() {
  const sel = document.getElementById('project');
  for (const opt of sel.options) opt.selected = true;
}

function clearForm() {
  document.getElementById('s1').value = '';
  document.getElementById('s2').value = '';
  const sel = document.getElementById('project');
  for (const opt of sel.options) opt.selected = false;
}

/*function openHelp() {
  window.open('help.jsp');
}*/

function goFirstProject() {
    const selectElement = document.getElementById('project');
    const selectedOptions = Array.from(selectElement.selectedOptions);
    if (selectedOptions.length > 0) {
        const firstProjectName = selectedOptions[0].text;
        window.location = "/folders?project=" + encodeURIComponent(firstProjectName);
    }
}
