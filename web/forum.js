"use strict";

const apiBase = document.querySelector('meta[name="akernel-api-base"]')
  .content.replace(/\/+$/, "");
const categoryNames = {
  kernel: "Kernel development",
  hardware: "Hardware & boot",
  help: "Help & questions",
  community: "Community",
};
const list = document.querySelector("#discussion-list");
const emptyState = document.querySelector("#empty-state");
const countLabel = document.querySelector("#discussion-count");
const apiNotice = document.querySelector("#api-notice");
const topicDialog = document.querySelector("#topic-dialog");
const newTopicDialog = document.querySelector("#new-topic-dialog");
const categoryToggle = document.querySelector("#categories-toggle");
const linksToggle = document.querySelector("#links-toggle");
const categoryPanel = document.querySelector("#categories-panel");
const linksPanel = document.querySelector("#links-panel");
let topics = [];
let selectedCategory = "all";
let selectedSort = "latest";
let activeTopicId = null;

function makeElement(tag, className, text) {
  const element = document.createElement(tag);
  if (className) element.className = className;
  if (text !== undefined) element.textContent = text;
  return element;
}

async function apiRequest(path, options = {}) {
  const headers = new Headers(options.headers || {});
  if (options.body !== undefined) headers.set("Content-Type", "application/json");
  let response;
  try {
    response = await fetch(`${apiBase}${path}`, {
      ...options,
      headers,
      credentials: "include",
    });
  } catch (error) {
    throw new Error(`Could not reach the forum API at ${apiBase}: ${error.message}`);
  }

  let payload;
  try {
    payload = await response.json();
  } catch {
    throw new Error(`The forum API returned an invalid response (${response.status}).`);
  }
  if (!response.ok) {
    throw new Error(payload.error || `Forum request failed (${response.status}).`);
  }
  return payload;
}

function formatDate(value) {
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return "just now";
  return new Intl.DateTimeFormat(undefined, {
    dateStyle: "medium",
    timeStyle: "short",
  }).format(date);
}

function getInitials(name) {
  return (name || "anon").trim().slice(0, 2).toUpperCase();
}

function renderTopics() {
  const query = document.querySelector("#search-input").value.trim().toLowerCase();
  const visibleTopics = topics.filter((topic) => {
    const matchesCategory = selectedCategory === "all" || topic.category === selectedCategory;
    const searchable = `${topic.title} ${topic.body} ${topic.author}`.toLowerCase();
    return matchesCategory && (!query || searchable.includes(query));
  });

  list.replaceChildren();
  countLabel.textContent = `${visibleTopics.length} ${visibleTopics.length === 1 ? "discussion" : "discussions"}`;
  emptyState.hidden = visibleTopics.length !== 0;
  list.hidden = visibleTopics.length === 0;

  for (const topic of visibleTopics) {
    const item = makeElement("article", "discussion-item");
    const avatar = makeElement("span", "avatar", getInitials(topic.author));
    avatar.setAttribute("aria-hidden", "true");

    const main = makeElement("div", "discussion-main");
    const title = makeElement("button", "discussion-title", topic.title);
    title.type = "button";
    title.addEventListener("click", () => openTopic(topic.id));
    const excerpt = makeElement("p", "discussion-excerpt", topic.body);
    const meta = makeElement("div", "discussion-meta");
    meta.append(
      makeElement("span", "meta-author", topic.author),
      makeElement("span", "meta-separator", "·"),
      makeElement("span", "", formatDate(topic.created_at)),
      makeElement("span", "meta-separator", "·"),
      makeElement("span", "category-pill", categoryNames[topic.category] || topic.category),
    );
    main.append(title, excerpt, meta);

    const stats = makeElement("div", "discussion-stats");
    stats.append(
      makeElement("span", "reply-count", String(topic.reply_count)),
      makeElement("span", "reply-label", topic.reply_count === 1 ? "reply" : "replies"),
    );
    item.append(avatar, main, stats);
    list.append(item);
  }
}

function renderTopicDetail(topic) {
  const detail = document.querySelector("#topic-detail");
  detail.replaceChildren();
  const header = makeElement("header", "topic-detail-heading");
  const category = makeElement("span", "category-pill", categoryNames[topic.category] || topic.category);
  const title = makeElement("h2", "", topic.title);
  title.id = "topic-dialog-title";
  const author = makeElement("span", "meta-author", topic.author);
  const date = makeElement("span", "reply-time", formatDate(topic.created_at));
  const body = makeElement("p", "topic-detail-body", topic.body);
  header.append(category, title, author, date, body);
  const replies = makeElement("div", "reply-list");
  for (const reply of topic.replies) {
    const card = makeElement("article", "reply-card");
    const avatar = makeElement("span", "avatar", getInitials(reply.author));
    avatar.setAttribute("aria-hidden", "true");
    const content = makeElement("div");
    const byline = makeElement("div");
    byline.append(
      makeElement("span", "reply-author", reply.author),
      makeElement("span", "reply-time", formatDate(reply.created_at)),
    );
    content.append(byline, makeElement("p", "", reply.body));
    card.append(avatar, content);
    replies.append(card);
  }
  if (topic.replies.length === 0) {
    replies.append(makeElement("p", "discussion-count", "No replies yet. Be the first to respond."));
  }
  detail.append(header, replies);
  if (!topicDialog.open) topicDialog.showModal();
}

async function openTopic(id) {
  activeTopicId = id;
  const detail = document.querySelector("#topic-detail");
  detail.replaceChildren(makeElement("p", "discussion-count", "Loading discussion…"));
  topicDialog.showModal();
  try {
    const topic = await apiRequest(`/topics/${encodeURIComponent(id)}`);
    renderTopicDetail(topic);
  } catch (error) {
    detail.replaceChildren(makeElement("p", "form-error", error.message));
  }
}

function showApiError(error) {
  document.querySelector("#api-error").textContent = error.message;
  apiNotice.hidden = false;
}

async function loadTopics() {
  const query = new URLSearchParams({ sort: selectedSort });
  if (selectedCategory !== "all") query.set("category", selectedCategory);
  const result = await apiRequest(`/topics?${query.toString()}`);
  topics = result.topics;
  renderTopics();
}

async function startForum() {
  try {
    const session = await apiRequest("/session", { method: "POST" });
    document.querySelector("#account-name").textContent = session.user.display_name;
    document.querySelector(".account-indicator").classList.add("connected");
    await loadTopics();
  } catch (error) {
    countLabel.textContent = "Forum offline";
    list.replaceChildren();
    showApiError(error);
  }
}

function openNewTopic() {
  document.querySelector("#topic-form-error").hidden = true;
  closeNavPanels();
  newTopicDialog.showModal();
}

function closeNavPanels() {
  categoryPanel.hidden = true;
  linksPanel.hidden = true;
  categoryToggle.setAttribute("aria-expanded", "false");
  linksToggle.setAttribute("aria-expanded", "false");
}

function toggleNavPanel(button, panel, otherButton, otherPanel) {
  const shouldOpen = panel.hidden;
  otherPanel.hidden = true;
  otherButton.setAttribute("aria-expanded", "false");
  panel.hidden = !shouldOpen;
  button.setAttribute("aria-expanded", String(shouldOpen));
}

categoryToggle.addEventListener("click", () => {
  toggleNavPanel(categoryToggle, categoryPanel, linksToggle, linksPanel);
});
linksToggle.addEventListener("click", () => {
  toggleNavPanel(linksToggle, linksPanel, categoryToggle, categoryPanel);
});
document.querySelector("#nav-new-topic").addEventListener("click", openNewTopic);
document.querySelector("#empty-new-topic").addEventListener("click", openNewTopic);
document.querySelectorAll("[data-close-dialog]").forEach((button) => {
  button.addEventListener("click", () => button.closest("dialog").close());
});
document.querySelectorAll(".category-link").forEach((button) => {
  button.addEventListener("click", async () => {
    selectedCategory = button.dataset.category;
    document.querySelectorAll(".category-link").forEach((entry) => {
      entry.classList.toggle("selected", entry.dataset.category === selectedCategory);
    });
    closeNavPanels();
    try {
      await loadTopics();
    } catch (error) {
      showApiError(error);
    }
  });
});
document.querySelectorAll(".feed-tab").forEach((button) => {
  button.addEventListener("click", async () => {
    selectedSort = button.dataset.sort;
    document.querySelectorAll(".feed-tab").forEach((entry) => {
      const active = entry === button;
      entry.classList.toggle("active", active);
      entry.setAttribute("aria-selected", String(active));
    });
    try {
      await loadTopics();
    } catch (error) {
      showApiError(error);
    }
  });
});
document.querySelector("#search-input").addEventListener("input", renderTopics);
document.addEventListener("keydown", (event) => {
  if (event.key === "/" && !["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) {
    event.preventDefault();
    document.querySelector("#search-input").focus();
  }
  if (event.key === "Escape") closeNavPanels();
});
document.addEventListener("click", (event) => {
  if (!event.target.closest(".nav-tools")) closeNavPanels();
});

document.querySelector("#new-topic-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = event.currentTarget;
  const submit = form.querySelector('[type="submit"]');
  const errorLabel = document.querySelector("#topic-form-error");
  errorLabel.hidden = true;
  submit.disabled = true;
  try {
    await apiRequest("/topics", {
      method: "POST",
      body: JSON.stringify({
        title: form.elements.title.value.trim(),
        category: form.elements.category.value,
        body: form.elements.body.value.trim(),
      }),
    });
    form.reset();
    newTopicDialog.close();
    selectedCategory = "all";
    document.querySelectorAll(".category-link").forEach((button) => {
      button.classList.toggle("selected", button.dataset.category === "all");
    });
    try {
      await loadTopics();
    } catch (error) {
      showApiError(error);
    }
  } catch (error) {
    errorLabel.textContent = error.message;
    errorLabel.hidden = false;
  } finally {
    submit.disabled = false;
  }
});

document.querySelector("#reply-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  if (!activeTopicId) return;
  const form = event.currentTarget;
  const submit = form.querySelector('[type="submit"]');
  const errorLabel = document.querySelector("#reply-form-error");
  errorLabel.hidden = true;
  submit.disabled = true;
  try {
    await apiRequest(`/topics/${encodeURIComponent(activeTopicId)}/replies`, {
      method: "POST",
      body: JSON.stringify({ body: form.elements.body.value.trim() }),
    });
    form.reset();
    try {
      const result = await apiRequest(`/topics/${encodeURIComponent(activeTopicId)}`);
      renderTopicDetail(result);
      await loadTopics();
    } catch (error) {
      showApiError(error);
    }
  } catch (error) {
    errorLabel.textContent = error.message;
    errorLabel.hidden = false;
  } finally {
    submit.disabled = false;
  }
});

startForum();
