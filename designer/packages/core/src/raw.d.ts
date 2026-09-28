// テストで見本のテキストを読み込む(Vite の ?raw の import。core は Node の fs を使わない)
declare module '*?raw' {
  const text: string;
  export default text;
}
