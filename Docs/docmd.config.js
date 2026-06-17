export default {
  title: "PocketMage",
  url: "https://ashtf8.github.io/PocketMage_PDA/docs",
  logo: { alt: "PocketMage", href: "./" },
  favicon: "/assets/favicon.png",
  theme: {
    name: "ruby",
    defaultMode: "system",
    enableModeToggle: true,
    positionMode: "top",
    codeHighlight: true,
    customCss: ["/assets/css/theme.css"],
    copyWidgets: {
      enabled: true,
      raw: true,
      context: true,
    },
  },
  layout: {
    footer: {
      style: "complete",
      description: "A clamshell PDA powered by an ESP32-S3 with E-Ink and OLED displays.",
      branding: true,
      columns: [
        {
          title: "Resources",
          links: [
            { text: "Command Manual", url: "./command-manual/" },
            { text: "FAQ", url: "./faq/" },
            { text: "Build Environments", url: "./development/build-environments" },
          ],
        },
        {
          title: "Community",
          links: [
            { text: "GitHub", url: "https://github.com/ashtf8/PocketMage_PDA" },
            { text: "Discord", url: "https://discord.gg/KSCapSf4XH" },
            { text: "Website", url: "https://pocketmage.org/" },
          ],
        },
      ],
    },
  },
  plugins: {
    search: {
      semantic: true,
      showConfidence: true,
    },
    seo: {
      defaultDescription:
        "PocketMage is an open-source clamshell PDA powered by an ESP32-S3 with E-Ink and OLED displays. Notes, calendar, tasks, and more.",
      openGraph: { defaultImage: "/assets/images/og-image.png" },
      twitter: { cardType: "summary_large_image" },
    },
    sitemap: {
      defaultChangefreq: "weekly",
      defaultPriority: 0.8,
    },
    mermaid: {},
    git: {},
    llms: {
      fullContext: true,
    },
  },
  search: true,
  minify: true,
  autoTitleFromH1: true,
  copyCode: true,
  pageNavigation: true,
  navigation: [
    { title: "Home", path: "/", icon: "home" },
    { title: "FAQ", path: "/faq/index", icon: "help-circle" },
    { title: "Command Manual", path: "/command-manual/index", icon: "keyboard" },
    {
      title: "Tutorials",
      icon: "book-open",
      collapsible: true,
      path: "/tutorials/index",
      children: [
        { title: "Format MicroSD Card", path: "/tutorials/format-micro-sd", icon: "card-sd" },
        { title: "PlatformIO Config", path: "/tutorials/platformio-config", icon: "settings" },
      ],
    },
    {
      title: "Development",
      icon: "code",
      collapsible: true,
      path: "/development/index",
      children: [
        { title: "Build Environments", path: "/development/build-environments", icon: "settings" },
      ],
    },
    {
      title: "Scripting",
      icon: "terminal",
      collapsible: true,
      path: "/scripting/index",
      children: [
        { title: "Basic Input/Output", path: "/scripting/example-c", icon: "terminal" },
        { title: "E-Ink Drawing", path: "/scripting/ink-c", icon: "image" },
        { title: "OLED Drawing", path: "/scripting/oled-c", icon: "monitor" },
        { title: "Full Command Reference", path: "/scripting/fullPotionCommandList", icon: "book" },
      ],
    },
    {
      title: "GitHub",
      path: "https://github.com/ashtf8/PocketMage_PDA",
      icon: "github",
      external: true,
    },
  ],
  footer: "Built with [docmd](https://docmd.io). [View on GitHub](https://github.com/ashtf8/PocketMage_PDA).",
  editLink: {
    enabled: true,
    baseUrl: "https://github.com/ashtf8/PocketMage_PDA/edit/main/Docs/docs",
    text: "Edit this page",
  },
};
