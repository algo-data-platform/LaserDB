(window.webpackJsonp=window.webpackJsonp||[]).push([[42,43],{100:function(e,t,n){"use strict";n.d(t,"a",(function(){return p})),n.d(t,"b",(function(){return y}));var a=n(0),r=n.n(a);function o(e,t,n){return t in e?Object.defineProperty(e,t,{value:n,enumerable:!0,configurable:!0,writable:!0}):e[t]=n,e}function l(e,t){var n=Object.keys(e);if(Object.getOwnPropertySymbols){var a=Object.getOwnPropertySymbols(e);t&&(a=a.filter((function(t){return Object.getOwnPropertyDescriptor(e,t).enumerable}))),n.push.apply(n,a)}return n}function c(e){for(var t=1;t<arguments.length;t++){var n=null!=arguments[t]?arguments[t]:{};t%2?l(Object(n),!0).forEach((function(t){o(e,t,n[t])})):Object.getOwnPropertyDescriptors?Object.defineProperties(e,Object.getOwnPropertyDescriptors(n)):l(Object(n)).forEach((function(t){Object.defineProperty(e,t,Object.getOwnPropertyDescriptor(n,t))}))}return e}function s(e,t){if(null==e)return{};var n,a,r=function(e,t){if(null==e)return{};var n,a,r={},o=Object.keys(e);for(a=0;a<o.length;a++)n=o[a],t.indexOf(n)>=0||(r[n]=e[n]);return r}(e,t);if(Object.getOwnPropertySymbols){var o=Object.getOwnPropertySymbols(e);for(a=0;a<o.length;a++)n=o[a],t.indexOf(n)>=0||Object.prototype.propertyIsEnumerable.call(e,n)&&(r[n]=e[n])}return r}var i=r.a.createContext({}),u=function(e){var t=r.a.useContext(i),n=t;return e&&(n="function"==typeof e?e(t):c(c({},t),e)),n},p=function(e){var t=u(e.components);return r.a.createElement(i.Provider,{value:t},e.children)},m={inlineCode:"code",wrapper:function(e){var t=e.children;return r.a.createElement(r.a.Fragment,{},t)}},d=r.a.forwardRef((function(e,t){var n=e.components,a=e.mdxType,o=e.originalType,l=e.parentName,i=s(e,["components","mdxType","originalType","parentName"]),p=u(n),d=a,y=p["".concat(l,".").concat(d)]||p[d]||m[d]||o;return n?r.a.createElement(y,c(c({ref:t},i),{},{components:n})):r.a.createElement(y,c({ref:t},i))}));function y(e,t){var n=arguments,a=t&&t.mdxType;if("string"==typeof e||a){var o=n.length,l=new Array(o);l[0]=d;var c={};for(var s in t)hasOwnProperty.call(t,s)&&(c[s]=t[s]);c.originalType=e,c.mdxType="string"==typeof e?e:a,l[1]=c;for(var i=2;i<o;i++)l[i]=n[i];return r.a.createElement.apply(null,l)}return r.a.createElement.apply(null,n)}d.displayName="MDXCreateElement"},115:function(e,t,n){"use strict";const a=(e,{target:t=document.body}={})=>{const n=document.createElement("textarea"),a=document.activeElement;n.value=e,n.setAttribute("readonly",""),n.style.contain="strict",n.style.position="absolute",n.style.left="-9999px",n.style.fontSize="12pt";const r=document.getSelection();let o=!1;r.rangeCount>0&&(o=r.getRangeAt(0)),t.append(n),n.select(),n.selectionStart=0,n.selectionEnd=e.length;let l=!1;try{l=document.execCommand("copy")}catch(c){}return n.remove(),o&&(r.removeAllRanges(),r.addRange(o)),a&&a.focus(),l};e.exports=a,e.exports.default=a},116:function(e,t){e.exports.parse=function(e){var t=e.split(",").map((function(e){return function(e){if(/^-?\d+$/.test(e))return parseInt(e,10);var t;if(t=e.match(/^(-?\d+)(-|\.\.\.?|\u2025|\u2026|\u22EF)(-?\d+)$/)){var n=t[1],a=t[2],r=t[3];if(n&&r){var o=[],l=(n=parseInt(n))<(r=parseInt(r))?1:-1;"-"!=a&&".."!=a&&"\u2025"!=a||(r+=l);for(var c=n;c!=r;c+=l)o.push(c);return o}}return[]}(e)}));return 0===t.length?[]:1===t.length?Array.isArray(t[0])?t[0]:t:t.reduce((function(e,t){return Array.isArray(e)||(e=[e]),Array.isArray(t)||(t=[t]),e.concat(t)}))}},136:function(e,t,n){"use strict";var a=n(2),r=n(0),o=n.n(r),l=n(101),c=n(102),s={plain:{backgroundColor:"#2a2734",color:"#9a86fd"},styles:[{types:["comment","prolog","doctype","cdata","punctuation"],style:{color:"#6c6783"}},{types:["namespace"],style:{opacity:.7}},{types:["tag","operator","number"],style:{color:"#e09142"}},{types:["property","function"],style:{color:"#9a86fd"}},{types:["tag-id","selector","atrule-id"],style:{color:"#eeebff"}},{types:["attr-name"],style:{color:"#c4b9fe"}},{types:["boolean","string","entity","url","attr-value","keyword","control","directive","unit","statement","regex","at-rule","placeholder","variable"],style:{color:"#ffcc99"}},{types:["deleted"],style:{textDecorationLine:"line-through"}},{types:["inserted"],style:{textDecorationLine:"underline"}},{types:["italic"],style:{fontStyle:"italic"}},{types:["important","bold"],style:{fontWeight:"bold"}},{types:["important"],style:{color:"#c4b9fe"}}]},i={Prism:n(19).a,theme:s};function u(e,t,n){return t in e?Object.defineProperty(e,t,{value:n,enumerable:!0,configurable:!0,writable:!0}):e[t]=n,e}function p(){return p=Object.assign||function(e){for(var t=1;t<arguments.length;t++){var n=arguments[t];for(var a in n)Object.prototype.hasOwnProperty.call(n,a)&&(e[a]=n[a])}return e},p.apply(this,arguments)}var m=/\r\n|\r|\n/,d=function(e){0===e.length?e.push({types:["plain"],content:"\n",empty:!0}):1===e.length&&""===e[0].content&&(e[0].content="\n",e[0].empty=!0)},y=function(e,t){var n=e.length;return n>0&&e[n-1]===t?e:e.concat(t)},h=function(e,t){var n=e.plain,a=Object.create(null),r=e.styles.reduce((function(e,n){var a=n.languages,r=n.style;return a&&!a.includes(t)||n.types.forEach((function(t){var n=p({},e[t],r);e[t]=n})),e}),a);return r.root=n,r.plain=p({},n,{backgroundColor:null}),r};function b(e,t){var n={};for(var a in e)Object.prototype.hasOwnProperty.call(e,a)&&-1===t.indexOf(a)&&(n[a]=e[a]);return n}var g=function(e){function t(){for(var t=this,n=[],a=arguments.length;a--;)n[a]=arguments[a];e.apply(this,n),u(this,"getThemeDict",(function(e){if(void 0!==t.themeDict&&e.theme===t.prevTheme&&e.language===t.prevLanguage)return t.themeDict;t.prevTheme=e.theme,t.prevLanguage=e.language;var n=e.theme?h(e.theme,e.language):void 0;return t.themeDict=n})),u(this,"getLineProps",(function(e){var n=e.key,a=e.className,r=e.style,o=p({},b(e,["key","className","style","line"]),{className:"token-line",style:void 0,key:void 0}),l=t.getThemeDict(t.props);return void 0!==l&&(o.style=l.plain),void 0!==r&&(o.style=void 0!==o.style?p({},o.style,r):r),void 0!==n&&(o.key=n),a&&(o.className+=" "+a),o})),u(this,"getStyleForToken",(function(e){var n=e.types,a=e.empty,r=n.length,o=t.getThemeDict(t.props);if(void 0!==o){if(1===r&&"plain"===n[0])return a?{display:"inline-block"}:void 0;if(1===r&&!a)return o[n[0]];var l=a?{display:"inline-block"}:{},c=n.map((function(e){return o[e]}));return Object.assign.apply(Object,[l].concat(c))}})),u(this,"getTokenProps",(function(e){var n=e.key,a=e.className,r=e.style,o=e.token,l=p({},b(e,["key","className","style","token"]),{className:"token "+o.types.join(" "),children:o.content,style:t.getStyleForToken(o),key:void 0});return void 0!==r&&(l.style=void 0!==l.style?p({},l.style,r):r),void 0!==n&&(l.key=n),a&&(l.className+=" "+a),l})),u(this,"tokenize",(function(e,t,n,a){var r={code:t,grammar:n,language:a,tokens:[]};e.hooks.run("before-tokenize",r);var o=r.tokens=e.tokenize(r.code,r.grammar,r.language);return e.hooks.run("after-tokenize",r),o}))}return e&&(t.__proto__=e),t.prototype=Object.create(e&&e.prototype),t.prototype.constructor=t,t.prototype.render=function(){var e=this.props,t=e.Prism,n=e.language,a=e.code,r=e.children,o=this.getThemeDict(this.props),l=t.languages[n];return r({tokens:function(e){for(var t=[[]],n=[e],a=[0],r=[e.length],o=0,l=0,c=[],s=[c];l>-1;){for(;(o=a[l]++)<r[l];){var i=void 0,u=t[l],p=n[l][o];if("string"==typeof p?(u=l>0?u:["plain"],i=p):(u=y(u,p.type),p.alias&&(u=y(u,p.alias)),i=p.content),"string"==typeof i){var h=i.split(m),b=h.length;c.push({types:u,content:h[0]});for(var g=1;g<b;g++)d(c),s.push(c=[]),c.push({types:u,content:h[g]})}else l++,t.push(u),n.push(i),a.push(0),r.push(i.length)}l--,t.pop(),n.pop(),a.pop(),r.pop()}return d(c),s}(void 0!==l?this.tokenize(t,a,l,n):[a]),className:"prism-code language-"+n,style:void 0!==o?o.root:{},getLineProps:this.getLineProps,getTokenProps:this.getTokenProps})},t}(r.Component),f=g,v=n(115),k=n.n(v),j=n(116),O=n.n(j),E={plain:{color:"#bfc7d5",backgroundColor:"#292d3e"},styles:[{types:["comment"],style:{color:"rgb(105, 112, 152)",fontStyle:"italic"}},{types:["string","inserted"],style:{color:"rgb(195, 232, 141)"}},{types:["number"],style:{color:"rgb(247, 140, 108)"}},{types:["builtin","char","constant","function"],style:{color:"rgb(130, 170, 255)"}},{types:["punctuation","selector"],style:{color:"rgb(199, 146, 234)"}},{types:["variable"],style:{color:"rgb(191, 199, 213)"}},{types:["class-name","attr-name"],style:{color:"rgb(255, 203, 107)"}},{types:["tag","deleted"],style:{color:"rgb(255, 85, 114)"}},{types:["operator"],style:{color:"rgb(137, 221, 255)"}},{types:["boolean"],style:{color:"rgb(255, 88, 116)"}},{types:["keyword"],style:{fontStyle:"italic"}},{types:["doctype"],style:{color:"rgb(199, 146, 234)",fontStyle:"italic"}},{types:["namespace"],style:{color:"rgb(178, 204, 214)"}},{types:["url"],style:{color:"rgb(221, 221, 221)"}}]},N=n(111),w=n(103);var x=()=>{const{prism:e}=Object(w.a)(),{isDarkTheme:t}=Object(N.a)(),n=e.theme||E,a=e.darkTheme||n;return t?a:n},C=n(49),P=n.n(C);const _=/{([\d,-]+)}/,T=function(e){void 0===e&&(e=["js","jsBlock","jsx","python","html"]);const t={js:{start:"\\/\\/",end:""},jsBlock:{start:"\\/\\*",end:"\\*\\/"},jsx:{start:"\\{\\s*\\/\\*",end:"\\*\\/\\s*\\}"},python:{start:"#",end:""},html:{start:"\x3c!--",end:"--\x3e"}},n=["highlight-next-line","highlight-start","highlight-end"].join("|"),a=e.map((e=>`(?:${t[e].start}\\s*(${n})\\s*${t[e].end})`)).join("|");return new RegExp(`^\\s*(?:${a})\\s*$`)},S=/title=".*"/;var D=e=>{let{children:t,className:n,metastring:l}=e;const{prism:s}=Object(w.a)(),[u,p]=Object(r.useState)(!1),[m,d]=Object(r.useState)(!1);Object(r.useEffect)((()=>{d(!0)}),[]);const y=Object(r.useRef)(null);let h=[],b="";const g=x();if(l&&_.test(l)){const e=l.match(_)[1];h=O.a.parse(e).filter((e=>e>0))}l&&S.test(l)&&(b=l.match(S)[0].split("title=")[1].replace(/"+/g,""));let v=n&&n.replace(/language-/,"");!v&&s.defaultLanguage&&(v=s.defaultLanguage);let j=t.replace(/\n$/,"");if(0===h.length&&void 0!==v){let e="";const n=(e=>{switch(e){case"js":case"javascript":case"ts":case"typescript":return T(["js","jsBlock"]);case"jsx":case"tsx":return T(["js","jsBlock","jsx"]);case"html":return T(["js","jsBlock","html"]);case"python":case"py":return T(["python"]);default:return T()}})(v),a=t.replace(/\n$/,"").split("\n");let r;for(let t=0;t<a.length;){const o=t+1,l=a[t].match(n);if(null!==l){switch(l.slice(1).reduce(((e,t)=>e||t),void 0)){case"highlight-next-line":e+=`${o},`;break;case"highlight-start":r=o;break;case"highlight-end":e+=`${r}-${o-1},`}a.splice(t,1)}else t+=1}h=O.a.parse(e),j=a.join("\n")}const E=()=>{k()(j),p(!0),setTimeout((()=>p(!1)),2e3)};return o.a.createElement(f,Object(a.a)({},i,{key:String(m),theme:g,code:j,language:v}),(e=>{let{className:t,style:n,tokens:r,getLineProps:l,getTokenProps:s}=e;return o.a.createElement(o.a.Fragment,null,b&&o.a.createElement("div",{style:n,className:P.a.codeBlockTitle},b),o.a.createElement("div",{className:P.a.codeBlockContent},o.a.createElement("button",{tabIndex:0,ref:y,type:"button","aria-label":"Copy code to clipboard",className:Object(c.a)(P.a.copyButton,{[P.a.copyButtonWithTitle]:b}),onClick:E},u?"Copied":"Copy"),o.a.createElement("div",{className:Object(c.a)(t,P.a.codeBlock,{[P.a.codeBlockWithTitle]:b})},o.a.createElement("div",{className:P.a.codeBlockLines,style:n},r.map(((e,t)=>{1===e.length&&""===e[0].content&&(e[0].content="\n");const n=l({line:e,key:t});return h.includes(t+1)&&(n.className=`${n.className} docusaurus-highlight-code-line`),o.a.createElement("div",Object(a.a)({key:t},n),e.map(((e,t)=>o.a.createElement("span",Object(a.a)({key:t},s({token:e,key:t}))))))}))))))}))},I=(n(50),n(51)),L=n.n(I);var M=e=>function(t){let{id:n,...a}=t;const{navbar:{hideOnScroll:r}}=Object(w.a)();return n?o.a.createElement(e,a,o.a.createElement("a",{"aria-hidden":"true",tabIndex:-1,className:Object(c.a)("anchor",{[L.a.enhancedAnchor]:!r}),id:n}),a.children,o.a.createElement("a",{"aria-hidden":"true",tabIndex:-1,className:"hash-link",href:`#${n}`,title:"Direct link to heading"},"#")):o.a.createElement(e,a)},B=n(52),A=n.n(B);const $={code:e=>{const{children:t}=e;return"string"==typeof t?t.includes("\n")?o.a.createElement(D,e):o.a.createElement("code",e):t},a:e=>o.a.createElement(l.a,e),pre:e=>o.a.createElement("div",Object(a.a)({className:A.a.mdxCodeBlock},e)),h1:M("h1"),h2:M("h2"),h3:M("h3"),h4:M("h4"),h5:M("h5"),h6:M("h6")};t.a=$},138:function(e,t,n){"use strict";n.r(t);var a=n(0),r=n.n(a),o=n(107);t.default=function(){return r.a.createElement(o.a,{title:"Page Not Found"},r.a.createElement("div",{className:"container margin-vert--xl"},r.a.createElement("div",{className:"row"},r.a.createElement("div",{className:"col col--6 col--offset-3"},r.a.createElement("h1",{className:"hero__title"},"Page Not Found"),r.a.createElement("p",null,"We could not find what you were looking for."),r.a.createElement("p",null,"Please contact the owner of the site that linked you to the original URL and let them know their link is broken.")))))}},97:function(e,t,n){"use strict";n.r(t);var a=n(0),r=n.n(a),o=n(100),l=n(104),c=n(21),s=n(107),i=n(2),u=n(102),p=n(103),m=n(146),d=n(139),y=n(143),h=n(144),b=n(145),g=n(142),f=n(101),v=n(110),k=n(63),j=n.n(k);const O=(e,t)=>"link"===e.type?Object(m.a)(e.href,t):"category"===e.type&&e.items.some((e=>O(e,t)));function E(e){let{item:t,onItemClick:n,collapsible:o,activePath:l,...c}=e;const{items:s,label:p}=t,m=O(t,l),d=function(e){const t=Object(a.useRef)(e);return Object(a.useEffect)((()=>{t.current=e}),[e]),t.current}(m),[y,h]=Object(a.useState)((()=>!!o&&(!m&&t.collapsed)));Object(a.useEffect)((()=>{m&&!d&&y&&h(!1)}),[m,d,y]);const b=Object(a.useCallback)((e=>{e.preventDefault(),h((e=>!e))}),[h]);return 0===s.length?null:r.a.createElement("li",{className:Object(u.a)("menu__list-item",{"menu__list-item--collapsed":y}),key:p},r.a.createElement("a",Object(i.a)({className:Object(u.a)("menu__link",{"menu__link--sublist":o,"menu__link--active":o&&m,[j.a.menuLinkText]:!o}),onClick:o?b:void 0,href:o?"#!":void 0},c),p),r.a.createElement("ul",{className:"menu__list"},s.map((e=>r.a.createElement(w,{tabIndex:y?"-1":"0",key:e.label,item:e,onItemClick:n,collapsible:o,activePath:l})))))}function N(e){let{item:t,onItemClick:n,activePath:a,collapsible:o,...l}=e;const{href:c,label:s}=t,p=O(t,a);return r.a.createElement("li",{className:"menu__list-item",key:s},r.a.createElement(f.a,Object(i.a)({className:Object(u.a)("menu__link",{"menu__link--active":p}),to:c},Object(v.a)(c)?{isNavLink:!0,exact:!0,onClick:n}:{target:"_blank",rel:"noreferrer noopener"},l),s))}function w(e){return"category"===e.item.type?r.a.createElement(E,e):r.a.createElement(N,e)}var x=function(e){let{path:t,sidebar:n,sidebarCollapsible:o=!0}=e;const[c,s]=Object(a.useState)(!1),{navbar:{title:m,hideOnScroll:v}}=Object(p.a)(),{isClient:k}=Object(l.a)(),{logoLink:O,logoLinkProps:E,logoImageUrl:N,logoAlt:x}=Object(b.a)(),{isAnnouncementBarClosed:C}=Object(d.a)(),{scrollY:P}=Object(g.a)();Object(y.a)(c);const _=Object(h.a)();return Object(a.useEffect)((()=>{_===h.b.desktop&&s(!1)}),[_]),r.a.createElement("div",{className:Object(u.a)(j.a.sidebar,{[j.a.sidebarWithHideableNavbar]:v})},v&&r.a.createElement(f.a,Object(i.a)({tabIndex:-1,className:j.a.sidebarLogo,to:O},E),null!=N&&r.a.createElement("img",{key:k,src:N,alt:x}),null!=m&&r.a.createElement("strong",null,m)),r.a.createElement("div",{className:Object(u.a)("menu","menu--responsive",j.a.menu,{"menu--show":c,[j.a.menuWithAnnouncementBar]:!C&&0===P})},r.a.createElement("button",{"aria-label":c?"Close Menu":"Open Menu","aria-haspopup":"true",className:"button button--secondary button--sm menu__button",type:"button",onClick:()=>{s(!c)}},c?r.a.createElement("span",{className:Object(u.a)(j.a.sidebarMenuIcon,j.a.sidebarMenuCloseIcon)},"\xd7"):r.a.createElement("svg",{"aria-label":"Menu",className:j.a.sidebarMenuIcon,xmlns:"http://www.w3.org/2000/svg",height:24,width:24,viewBox:"0 0 32 32",role:"img",focusable:"false"},r.a.createElement("title",null,"Menu"),r.a.createElement("path",{stroke:"currentColor",strokeLinecap:"round",strokeMiterlimit:"10",strokeWidth:"2",d:"M4 7h22M4 15h22M4 23h22"}))),r.a.createElement("ul",{className:"menu__list"},n.map((e=>r.a.createElement(w,{key:e.label,item:e,onItemClick:e=>{e.target.blur(),s(!1)},collapsible:o,activePath:t}))))))},C=n(136),P=n(138),_=n(108),T=n(64),S=n.n(T),D=n(147);function I(e){var t,n;let{currentDocRoute:a,versionMetadata:c,children:i}=e;const{siteConfig:u,isClient:p}=Object(l.a)(),{pluginId:m,permalinkToSidebar:d,docsSidebars:y,version:h}=c,b=d[a.path],g=y[b];return r.a.createElement(s.a,{key:p,searchMetadatas:{version:h,tag:Object(D.b)(m,h)}},r.a.createElement("div",{className:S.a.docPage},g&&r.a.createElement("div",{className:S.a.docSidebarContainer,role:"complementary"},r.a.createElement(x,{key:b,sidebar:g,path:a.path,sidebarCollapsible:null===(t=null===(n=u.themeConfig)||void 0===n?void 0:n.sidebarCollapsible)||void 0===t||t})),r.a.createElement("main",{className:S.a.docMainContainer},r.a.createElement(o.a,{components:C.a},i))))}t.default=function(e){const{route:{routes:t},versionMetadata:n,location:a}=e,o=t.find((e=>Object(_.matchPath)(a.pathname,e)));return o?r.a.createElement(I,{currentDocRoute:o,versionMetadata:n},Object(c.a)(t)):r.a.createElement(P.default,e)}}}]);