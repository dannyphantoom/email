import * as React from 'react';
import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { Toaster } from 'react-hot-toast';
import { useAuthStore } from './stores/authStore';
import Login from './pages/Login';
import Register from './pages/Register';
import Messenger from './pages/Messenger';
import AccountIntegration from './pages/AccountIntegration';
import UnifiedInbox from './pages/UnifiedInbox';
import './index.css';

const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      retry: 1,
      refetchOnWindowFocus: false,
    },
  },
});

function App() {
  const { isAuthenticated } = useAuthStore();

  return (
    <QueryClientProvider client={queryClient}>
      <Router>
        <div className="min-h-screen bg-gradient-to-br from-dark-950 via-dark-900 to-cockpit-950">
          <Routes>
            <Route 
              path="/login" 
              element={isAuthenticated ? <Navigate to="/messenger" /> : <Login />} 
            />
            <Route 
              path="/register" 
              element={isAuthenticated ? <Navigate to="/messenger" /> : <Register />} 
            />
            <Route 
              path="/messenger/*" 
              element={isAuthenticated ? <Messenger /> : <Navigate to="/login" />} 
            />
            <Route 
              path="/integration" 
              element={isAuthenticated ? <AccountIntegration /> : <Navigate to="/login" />} 
            />
            <Route 
              path="/inbox" 
              element={isAuthenticated ? <UnifiedInbox /> : <Navigate to="/login" />} 
            />
            <Route 
              path="/" 
              element={<Navigate to={isAuthenticated ? "/messenger" : "/login"} />} 
            />
          </Routes>
        </div>
        
        <Toaster
          position="top-right"
          toastOptions={{
            duration: 4000,
            style: {
              background: '#1a1d23',
              color: '#fff',
              border: '1px solid #333945',
            },
            success: {
              iconTheme: {
                primary: '#10b981',
                secondary: '#fff',
              },
            },
            error: {
              iconTheme: {
                primary: '#ef4444',
                secondary: '#fff',
              },
            },
          }}
        />
      </Router>
    </QueryClientProvider>
  );
}

export default App; 