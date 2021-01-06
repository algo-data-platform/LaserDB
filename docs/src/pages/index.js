import React from 'react';
import clsx from 'clsx';
import Layout from '@theme/Layout';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import useBaseUrl from '@docusaurus/useBaseUrl';
import styles from './styles.module.css';

const features = [
  {
    title: 'High Performance',
    imageUrl: 'img/performance.svg',
    description: (
      <>
        High performance, low latency and high throughput for online computing. P999 less then 3ms in most scenarios, and provides efficient parallel interfaces.
      </>
    ),
  },
  {
    title: 'Huge Capacity',
    imageUrl: 'img/capacity.svg',
    description: (
      <>
        Break through the memory capacity bottleneck, a single cluster can easily carry hundreds of TB of data, to meet the needs of machine learning, big data processing.
      </>
    ),
  },
  {
    title: 'Rapid Batch Import',
    imageUrl: 'img/batch_import.svg',
    description: (
      <>
        Rapidly import billions of pieces of data from offline environment into, to meet the requirement of frequently data update in machine learning and big data processing.
      </>
    ),
  },
];

function Feature({ imageUrl, title, description }) {
  const imgUrl = useBaseUrl(imageUrl);
  return (
    <div className={clsx('col col--4', styles.feature)}>
      {imgUrl && (
        <div className="text--center">
          <img className={styles.featureImage} src={imgUrl} alt={title} />
        </div>
      )}
      <h3>{title}</h3>
      <p>{description}</p>
    </div>
  );
}

function Home() {
  const context = useDocusaurusContext();
  const { siteConfig = {} } = context;
  return (
    <Layout
      title={`${siteConfig.title}`}
      description="Description will go into a meta tag in <head />">
      <header className={clsx('hero hero--primary', styles.heroBanner)} style={{ backgroundImage: "url('/img/background.svg')", backgroundSize: 'cover', backgroundPosition: 'center', backgroundRepeat: 'no-repeat' }}>
        <div className="container">
          <h1 className="hero__title">{siteConfig.title}</h1>
          <p className="hero__subtitle">{siteConfig.tagline}</p>
          <div className={styles.buttons}>
            <Link
              className={clsx(
                'button button--outline button--secondary button--lg',
                styles.getStarted,
              )}
              to={useBaseUrl('docs/')}>
              Get Started
            </Link>
          </div>
        </div>
      </header>
      <main>
        {features && features.length > 0 && (
          <section className={styles.features}>
            <div className="container">
              <div className="row">
                {features.map((props, idx) => (
                  <Feature key={idx} {...props} />
                ))}
              </div>
            </div>
          </section>
        )}
      </main>
    </Layout >
  );
}

export default Home;
